#include "directory.h"
#include "hash.h"
#include "cache.h"
#include "inode.h"
#include "btr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Directory operations
int directory_create(DiskInterface* disk, uint64_t parent_inode, const char* name, uint64_t* new_inode)
{
    
}

int directory_lookup(DiskInterface* disk, uint64_t dir_inode, const char* name, uint64_t* found_inode)
{
    // TODO: Perhaps decide if this is really necessary, as we will do lokups through our B-Tree...
}

int directory_add_entry(DiskInterface* disk, cache *cache, const char *path, const char* name, uint64_t target_inode, FileType type)
{
    int rv = -1;
    InodeBtreePair *pair = item_search(disk, cache, path);
    Inode node = {0};
    uint64_t block;
    block_type_t *block_type;
    DirectoryBlock *db;
    DirEntry *entry;
    DirEntry new_file;
    
    new_file.inode_number = target_inode;
    strcpy(new_file.name, name);
    new_file.active = true;
    
    if (pair->inode_number || !strcmp(path, "/"))
    {
        inode_read(disk, cache, pair->inode_number, &node);
        for (uint16_t i=0; i < ( ( UINT16_MAX * sizeof(struct DirEntry) ) / USABLE_BLOCK_SIZE ); i++)
        {
            inode_get_block(disk, cache, &node, i, &block);
            if (!block)
            {
                block = alloc_page(disk, cache);
                inode_set_block(disk, cache, &node, i, block);
                block_type = get_block(disk, cache, pair->inode_number, block);
                *block_type = BLOCK_TYPE_DATA;
                inode_write(disk, cache, &node);
            }
            else block_type = get_block(disk, cache, pair->inode_number, block);
            if (BLOCK_TYPE_DATA != *block_type)
            {
                fprintf(stderr, "ERROR: Not a data type block!!\n");
                rv = -1;
                goto free_pair;
            }
            if (0 == i)
            {
                db = (DirectoryBlock*) ( block_type + 1 );
                entry = (DirEntry*) ( db + 1 );
            }
            else entry = (DirEntry*) ( block_type + 1 );
            for (uint16_t j=0; ( j % ( USABLE_BLOCK_SIZE / sizeof(struct DirEntry) ) ) != 0 || !j ; j++)
            {
                if (!entry->active || j == db->entry_count)
                {
                    db->entry_count++;
                    if (FILE_TYPE_DIRECTORY == type)
                    {
                        uint64_t dir_root_page;
                        BTreeNode *dir_root = btree_node_create(disk, cache, false, &dir_root_page);
                        btree_insert(disk, cache, pair->btree_block, path_hash(name), dir_root_page);
                        dir_root->value = target_inode;
                        new_file.btree_block = dir_root_page;
                    }
                    else btree_insert(disk, cache, pair->btree_block, path_hash(name), target_inode);
                    memcpy( entry, &new_file, sizeof(struct DirEntry) );
                    rv = 0;
                    goto free_pair;
                }
                entry++;
            }
        }
    }
free_pair:
    free(pair);
    return rv;
}

int directory_remove_entry(DiskInterface* disk, cache *cache, const char *path, const char* name)
{
    int rv = -1;
    InodeBtreePair *pair = item_search(disk, cache, path);
    Inode dir_node = {0}, file_node = {0};
    uint64_t block;
    block_type_t *block_type;
    DirectoryBlock *db;
    DirEntry *entry;
    
    uint16_t count = 0;
    
    if (pair->inode_number || !strcmp(path, "/"))
    {
        inode_read(disk, cache, pair->inode_number, &dir_node);
        for (uint16_t i=0; i < ( ( UINT16_MAX * sizeof(struct DirEntry) ) / USABLE_BLOCK_SIZE ); i++)
        {
            inode_get_block(disk, cache, &dir_node, i, &block);
            if (!block)
                goto free_pair;
            block_type = get_block(disk, cache, pair->inode_number, block);
            if (BLOCK_TYPE_DATA != *block_type)
            {
                fprintf(stderr, "ERROR: Not a data type block!!\n");
                goto free_pair;
            }
            if (0 == count)
            {
                db = (DirectoryBlock*) ( block_type + 1 );
                entry = (DirEntry*) ( db + 1 );
            }
            else entry = (DirEntry*) ( block_type + 1 );
            if (db->entry_count == count) break;
            for (uint16_t j=0; ( j % ( USABLE_BLOCK_SIZE / sizeof(struct DirEntry) ) ) != 0 || !j ; j++)
            {
                if (!strcmp(entry->name, name))
                {
                    entry->active = false;
                    db->entry_count--;
                    inode_read(disk, cache, entry->inode_number, &file_node);
                    if (1 == file_node.reference_count)
                        if (inode_free(disk, cache, entry->inode_number))
                            goto free_pair;
                    rv = 0;
                    break;
                }
                entry++;
                count++;
            }
        }
    }
free_pair:
    free(pair);
    return rv;
}
int directory_list(DiskInterface* disk, cache *cache, const char *path, DirEntry** entries)
{
    int rv = 0;
    InodeBtreePair *pair = item_search(disk, cache, path);
    Inode node = {0};
    uint64_t block;
    block_type_t *block_type;
    DirectoryBlock *db;
    DirEntry *entry;
    
    if (pair->inode_number || !strcmp(path, "/"))
    {
        inode_read(disk, cache, pair->inode_number, &node);
        for (uint16_t i=0; i < ( ( UINT16_MAX * sizeof(struct DirEntry) ) / USABLE_BLOCK_SIZE ); i++)
        {
            inode_get_block(disk, cache, &node, i, &block);
            if (!block)
                goto free_pair;
            block_type = get_block(disk, cache, pair->inode_number, block);
            if (BLOCK_TYPE_DATA != *block_type)
            {
                fprintf(stderr, "ERROR: Not a data type block!!\n");
                goto free_pair;
            }
            if (0 == rv)
            {
                db = (DirectoryBlock*) ( block_type + 1 );
                *entries = malloc( db->entry_count * sizeof(struct DirEntry) );
                entry = (DirEntry*) ( db + 1 );
            }
            else entry = (DirEntry*) ( block_type + 1 );
            if (db->entry_count == rv) break;
            for (uint16_t j=0; ( j % ( USABLE_BLOCK_SIZE / sizeof(struct DirEntry) ) ) != 0 || !j ; j++)
            {
                if (entry->active)
                {
                    memcpy( ( *entries + ( rv * sizeof(struct DirEntry) ) ), entry, sizeof(struct DirEntry) );
                }
                rv++;
                entry++;
            }
        }
    }
free_pair:
    free(pair);
    return rv;
}
