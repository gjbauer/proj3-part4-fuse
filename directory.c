#include "directory.h"
#include "hash.h"
#include "cache.h"
#include <stdio.h>

// Directory operations
int directory_create(DiskInterface* disk, uint64_t parent_inode, const char* name, uint64_t* new_inode)
{
    
}

int directory_lookup(DiskInterface* disk, uint64_t dir_inode, const char* name, uint64_t* found_inode)
{
    // TODO: Perhaps decide if this is really necessary, as we will do lokups through our B-Tree...
}

int directory_add_entry(DiskInterface* disk, uint64_t dir_inode, const char* name, uint64_t target_inode, FileType type)
{
    
}
int directory_remove_entry(DiskInterface* disk, uint64_t dir_inode, const char* name)
{
    
}
int directory_list(DiskInterface* disk, cache *cache, uint64_t dir_inode, DirEntry** entries, int* count)
{
    int rv = 0;
    InodeBtreePair *pair = item_search(disk, cache_s, path);
    Inode node;
    uint64_t block;
    block_type_t *block_type;
    uint16_t number_of_entries;
    DirectoryBlock *db;
    DirEntry *entry;
    
    *count = 0;
    
    if (pair->inode_number || !strcmp(path, "/"))
    {
        inode_read(disk, cache_s, pair->inode_number, &node);
        for (uint16_t i=0; i < ( ( UINT16_MAX * sizeof(struct DirEntry) ) / USABLE_BLOCK_SIZE ); i++)
        {
            inode_get_block(disk, cache_s, &node, i, &block);
            if (!block)
                break;
            block_type = get_block(disk, cache, pair->inode_number, block);
            if (BLOCK_TYPE_DATA != *block_type)
            {
                fprintf(stderr, "ERROR: Not a data type block!!\n");
                rv = -1;
                break;
            }
            if (0 == *count)
            {
                db = (DirectoryBlock*) ( block_type + 1 );
                number_of_entries = db->entry_count;
                *entries = malloc( number_of_entries * sizeof(struct DirEntry) );
                entry = (DirEntry*) ( db + 1 );
            }
            else entry = (DirEntry*) ( block_type + 1 );
            if (number_of_entries == *count) break;
            for (uint16_t j=0; ( j % ( USABLE_BLOCK_SIZE / sizeof(struct DirEntry) ) ) != 0 || !j ; j++)
            {
                if (entry->active)
                {
                    memcpy( ( *entries + ( count * sizeof(struct DirEntry) ) ), entry, sizeof(struct DirEntry) );
                }
                *count++;
                entry++;
            }
        }
    }
    free(pair);
    return rv;
}
