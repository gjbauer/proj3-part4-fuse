#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "cache.h"
#include "btr.h"
#include "superblock.h"
#include "inode.h"
#include "hash.h"

int main()
{
	DiskInterface* disk = disk_open("my.img");
	cache *cache = NULL;
    Superblock superblock;

    if (superblock_initialize(disk, cache, "UNTITLED")) fprintf(stderr, "ERROR: Volume name too long\n");
    if (superblock_read(disk, cache, &superblock)) fprintf(stderr, "ERROR: Invalid superblock!\n");
    printf("Setting block types to bitmaps for bitmaps...\n");
    block_type_t *block_type;
    for (int i=1; i < superblock.inode_bitmap+calculate_inode_bitmap_size(&superblock); i++ )
    {
        block_type = (block_type_t*)get_block(disk, cache, 0, i);
        *block_type = BLOCK_TYPE_BITMAP;
    }
    printf("Setting block types to INODE for inode table blocks...\n");
    for (int i=superblock.inode_bitmap+calculate_inode_bitmap_size(&superblock); i < ( superblock.inode_bitmap+calculate_inode_bitmap_size(&superblock)+calculate_inode_table_size(&superblock) ); i++ )
    {
        block_type = (block_type_t*)get_block(disk, cache, 0, i);
        *block_type = BLOCK_TYPE_INODE;
    }
    printf("Allocating pages for superblock, bitmaps, and inode table...\n");
    for (int i=0; i < (superblock.inode_bitmap+calculate_inode_bitmap_size(&superblock)+calculate_inode_table_size(&superblock)) ; i++) alloc_page(disk, cache);
    superblock.free_blocks = superblock.total_blocks - (superblock.inode_bitmap+calculate_inode_bitmap_size(&superblock)+calculate_inode_table_size(&superblock));
    printf("Free blocks: %llu\n", superblock.free_blocks);
    printf("Usable block size / inode size : %lu\n", USABLE_BLOCK_SIZE/sizeof(struct Inode));

    printf("Creating root tree node...\n");
    uint64_t page;
    BTreeNode *root = btree_node_create(disk, cache, false, &page);
    root->value = inode_allocate(disk, cache, FILE_TYPE_DIRECTORY);
    superblock.btree_root = page;

    printf("Writing superblock...\n");
    superblock_write(disk, cache, &superblock);
}
