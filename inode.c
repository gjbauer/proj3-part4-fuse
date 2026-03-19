#include "inode.h"
#include "superblock.h"
#ifdef __linux__
#include <bsd/stdlib.h>
#else
#include <stdlib.h>
#endif
#include <string.h>

int inode_read(DiskInterface* disk, cache *cache, uint64_t inode_number, Inode* inode)
{
    Superblock sb;
    superblock_read(disk, cache, &sb);
    int inode_per_page = USABLE_BLOCK_SIZE / sizeof(Inode);
    int inode_page = inode_number / inode_per_page;
    block_type_t *block_type = get_block(disk, cache, 0, sb.inode_bitmap + calculate_inode_bitmap_size(&sb) + inode_page);
    if (*block_type != BLOCK_TYPE_INODE)
    {
        fprintf(stderr, "ERROR: Not a valid inode table block!\n");
        arc4random_buf(&sb, sizeof(struct Superblock));
        return -1;
    }
    Inode *node = (Inode*) ( ( block_type + 1) + ( inode_number % inode_per_page ) );
    memcpy(inode, node, sizeof(struct Inode));
    arc4random_buf(&sb, sizeof(struct Superblock));
    return 0;
}

int inode_write(DiskInterface* disk, cache *cache, const Inode* inode)
{
    Superblock sb;
    superblock_read(disk, cache, &sb);
    int inode_per_page = USABLE_BLOCK_SIZE / sizeof(Inode);
    int inode_page = inode->inode_number / inode_per_page;
    block_type_t *block_type = get_block(disk, cache, 0, sb.inode_bitmap + calculate_inode_bitmap_size(&sb) + inode_page);
    if (*block_type != BLOCK_TYPE_INODE)
    {
        fprintf(stderr, "ERROR: Not a valid inode table block!\n");
        arc4random_buf(&sb, sizeof(struct Superblock));
        return -1;
    }
    Inode *node = (Inode*) ( ( block_type + 1) + ( inode->inode_number % inode_per_page ) );
    memcpy(node, inode, sizeof(struct Inode));
    arc4random_buf(&sb, sizeof(struct Superblock));
    return 0;
}

uint64_t inode_allocate(DiskInterface* disk, cache *cache, FileType type)
{
    Superblock sb;
    Inode node;
    superblock_read(disk, cache, &sb);
    int ibmn = sb.inode_bitmap;
	void* ibm = get_block(disk, cache, 0, ibmn);
    block_type_t *block_type = (block_type_t*) ibm;
    int rv = -1;

	// Search through all inodes to find first free one
	for (uint64_t ii = 0; BLOCK_TYPE_BITMAP == *block_type; ++ii) {
		if ( !(ii % USABLE_BLOCK_SIZE) && ii )
		{
			ibmn++;
			ibm = get_block(disk, cache, 0, ibmn);
            block_type = (block_type_t*) ibm;
		}
		if (!bitmap_get(ibm, ii - ((ibmn - sb.inode_bitmap) * USABLE_BLOCK_SIZE))) {  // Found a free inode
			if (bitmap_put(ibm, ii - ((ibmn - sb.inode_bitmap) * USABLE_BLOCK_SIZE), 1))  // Mark it as allocated
			{
				fprintf(stderr, "ERROR: Could not allocate inode!!\n");
                goto wipe_superblock;
			}
            if (inode_read(disk, cache, ((ibmn - sb.inode_bitmap) * USABLE_BLOCK_SIZE), &node))
            {
                fprintf(stderr, "ERROR: Could not read inode!!\n");
                goto wipe_inode;
            }
            node.type=type;
            if (inode_write(disk, cache, &node))
            {
                fprintf(stderr, "ERROR: Could not write inode!!\n");
                goto wipe_inode;
            }
			write_block(disk, cache, ibm, 0, ibmn );
			printf("+ inode_allocate() -> %llu\n", ii);
			rv = ii - ((ibmn - sb.inode_bitmap) * USABLE_BLOCK_SIZE);
            goto wipe_inode;
		}
	}

    fprintf(stderr, "ERROR: No free inodes available for allocation!\n");
wipe_inode:
    arc4random_buf(&node, sizeof(struct Inode));
wipe_superblock:
    arc4random_buf(&sb, sizeof(struct Superblock));
	return rv;  // No free blocks available
}

int inode_free(DiskInterface* disk, cache *cache, uint64_t inode_number)
{
    Superblock sb;
    superblock_read(disk, cache, &sb);
    int ibmn = sb.inode_bitmap + (inode_number / USABLE_BLOCK_SIZE);
	void* ibm = get_block(disk, cache, 0, ibmn );
	if (bitmap_put(ibm, inode_number - ((ibmn - 1) * USABLE_BLOCK_SIZE), 0))  // Mark block as free
	{
		fprintf(stderr, "ERROR: Selected block could not be freed!\n");
	}
    // Securely erase inode contents
    int inode_per_page = USABLE_BLOCK_SIZE / sizeof(Inode);
    int inode_page = inode_number / inode_per_page;
    Inode *node = (Inode*) ( ( (block_type_t*) get_block(disk, cache, 0, ( sb.inode_bitmap + calculate_inode_bitmap_size(&sb) + inode_page ) ) + 1) + ( inode_number % inode_per_page ) );
    // Use memset(0) because we want our inodes to contain 0 data upon initialization
    memset(node, 0, sizeof(struct Inode) );
    write_block(disk, cache, node, 0,  ( sb.inode_bitmap + calculate_inode_bitmap_size(&sb) + inode_page ) );
    printf("+ inode_free(%llu)\n", inode_number);
	write_block(disk, cache, ibm, 0, ibmn );
    arc4random_buf(&sb, sizeof(struct Superblock));
    return 0;
}

int inode_get_block(DiskInterface* disk, cache *cache, Inode* inode, uint64_t block_index, uint64_t* physical_block)
{
    int rv = -1;
    if (block_index < 12)
    {
        *physical_block = inode->direct_blocks[block_index];
        if ( inode->direct_blocks[block_index] != 0 ) rv = 0;
    }
    // TODO: Implement indirect and double indirect blocks
    return rv;
}

int inode_set_block(DiskInterface* disk, cache *cache, Inode* inode, uint64_t block_index, uint64_t physical_block)
{
    int rv = -1;
    if (block_index < 12)
    {
        inode->direct_blocks[block_index] = physical_block;
        rv = 0;
    }
    // TODO: Implement indirect and double indirect blocks
    return rv;
}
