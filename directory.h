#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "config.h"
#include "disk.h"
#include <limits.h>
#include "types.h"

// Directory entry structure
typedef struct DirEntry {
    uint64_t inode_number;           // Inode number of the entry
    char name[NAME_MAX];             // Name of the entry
    FileType type;                   // Type of entry (file, dir, etc.)
} DirEntry;

// Directory block structure
typedef struct DirectoryBlock {
    uint16_t entry_count;            // Number of entries in this directory
} DirectoryBlock;

// Directory operations
int directory_create(DiskInterface* disk, uint64_t parent_inode, const char* name, uint64_t* new_inode);
int directory_lookup(DiskInterface* disk, uint64_t dir_inode, const char* name, uint64_t* found_inode);
int directory_add_entry(DiskInterface* disk, uint64_t dir_inode, const char* name, uint64_t target_inode, FileType type);
int directory_remove_entry(DiskInterface* disk, uint64_t dir_inode, const char* name);
int directory_list(DiskInterface* disk, uint64_t dir_inode, DirEntry** entries, int* count);

#endif

