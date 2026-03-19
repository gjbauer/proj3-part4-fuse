#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>

#include "disk.h"
#include "config.h"
#include "cache.h"

/**
 * Open and memory-map a disk image file for filesystem operations
 * Creates a DiskInterface structure for accessing the disk
 */
DiskInterface* disk_open(const char* filename)
{
	DiskInterface *disk = (DiskInterface*)malloc(sizeof(struct DiskInterface));
	struct stat fs_info;
	
	// Get file size and other metadata
	if (stat(filename, &fs_info) != 0) {
		fprintf(stderr, "Failed to stat filesystem!!");
		return NULL;
	}
	
	// Open the disk image file for read/write access
	disk->disk_file = open(filename, O_RDWR, 0644);
	assert(disk->disk_file != -1);
	
	// Memory-map the entire file for direct access
	disk->disk_base = mmap(0, fs_info.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, disk->disk_file, 0);
	assert(disk->disk_base != MAP_FAILED);
	
	// Calculate total number of blocks based on file size
	disk->total_blocks = fs_info.st_size / BLOCK_SIZE;
	
	return disk;
}

/**
 * Close the disk interface and clean up resources
 * Unmaps memory and closes file handle
 */
void disk_close(DiskInterface* disk)
{
	munmap(disk->disk_base, disk->total_blocks * BLOCK_SIZE);
	close(disk->disk_file);
	free(disk);
}

/**
 * Get a pointer to a specific block in the memory-mapped disk
 * Provides direct access to block data without copying
 */
void*
disk_get_block(DiskInterface* disk, int pnum)
{
	return disk->disk_base + BLOCK_SIZE * pnum;
}

/**
 * Allocate a free block from the filesystem
 * Searches the block bitmap for the first available block
 */
int
alloc_page(DiskInterface* disk, cache *cache)
{
	int pbmn = 1;
	void* pbm = get_block(disk, cache, 0, pbmn);

	// Search through all blocks to find first free one
	for (int ii = 0; ii < disk->total_blocks; ++ii) {
		if ( !(ii % USABLE_BLOCK_SIZE) && ii )
		{
			pbmn++;
			pbm = get_block(disk, cache, 0, pbmn);
		}
		if (!bitmap_get(pbm, ii - ((pbmn - 1) * USABLE_BLOCK_SIZE))) {  // Found a free block
			if (bitmap_put(pbm, ii - ((pbmn - 1) * USABLE_BLOCK_SIZE), 1))  // Mark it as allocated
			{
				fprintf(stderr, "ERROR: Could not allocate page!!\n");
				return -1;
			}
			write_block(disk, cache, pbm, 0, pbmn );
			printf("+ alloc_page() -> %d\n", ii);
			return ii - ((pbmn - 1) * USABLE_BLOCK_SIZE);
		}
	}

	fprintf(stderr, "ERROR: No free blocks available for allocation!\n");
	return -1;  // No free blocks available
}

/**
 * Free a previously allocated block
 * Marks the block as available in the block bitmap
 */
void
free_page(DiskInterface* disk, cache *cache, int pnum)
{
	int pbmn = 1 + (pnum / USABLE_BLOCK_SIZE);
	void* pbm = get_block(disk, cache, 0, pbmn );
	if (bitmap_put(pbm, pnum - ((pbmn - 1) * USABLE_BLOCK_SIZE), 0))  // Mark block as free
	{
		fprintf(stderr, "ERROR: Selected block could not be freed!\n");
	}
	printf("+ free_page(%d)\n", pnum);
	write_block(disk, cache, pbm, 0, pbmn );
}

/**
 * Read a block from disk into a buffer
 * Uses memory-mapped access for efficient copying
 */
int disk_read_block(DiskInterface* disk, uint64_t block_num, void* buffer)
{
	int rv = -1;
	void *block = disk_get_block(disk, block_num);
	
	// Copy block data to user buffer
	if (memcpy(buffer, block, BLOCK_SIZE)) {
		rv = 0;
	}
	
	return rv;
}

/**
 * Write buffer data to a block on disk
 * Uses memory-mapped access for efficient copying
 */
int disk_write_block(DiskInterface* disk, uint64_t block_num, const void* buffer)
{
	int rv = -1;
	void *block = disk_get_block(disk, block_num);
	
	// Copy user buffer to block location
	if (memcpy(block, buffer, BLOCK_SIZE)) {
		rv = 0;
	}
	
	return rv;
}

/**
 * Format the disk with a new filesystem
 * TODO: Implement filesystem formatting functionality
 */
int disk_format(DiskInterface* disk, const char* volume_name);
