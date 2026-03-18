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
int directory_list(DiskInterface* disk, uint64_t dir_inode, DirEntry** entries, int* count)
{
    
}
