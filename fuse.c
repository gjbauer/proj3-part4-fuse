#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef __linux__
#include <bsd/stdlib.h>
#endif
#include <stdlib.h>
#include <assert.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "hash.h"
#include "inode.h"
#include "string.h"
#include "directory.h"

// For debugging purposes
#include "btr.h"

DiskInterface* disk;
cache *cache_s;

// implementation for: man 2 access
// Checks if a file exists.
int
nbtrfs_access(const char *path, int mask)
{
    int rv = -ENOENT;
    InodeBtreePair *pair = item_search(disk, cache_s, path);
    if (pair->inode_number || pair->btree_block) rv = 0;
    printf("access(%s, %04o) -> %d\n", path, mask, rv);
    free(pair);
    return rv;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nbtrfs_getattr(const char *path, struct stat *st)
{
    int rv = -ENOENT;
    InodeBtreePair *pair = item_search(disk, cache_s, path);
    Inode node;
    if (pair->inode_number || pair->btree_block)
    {
        inode_read(disk, cache_s, pair->inode_number, &node);
        st->st_mode = node.mode;
        st->st_size = node.size;
        st->st_uid = node.owner_id;
        st->st_gid = getgid();
        st->st_nlink = node.reference_count;
        st->st_atime = st->st_mtime = st->st_ctime = time(NULL);
        st->st_ino = pair->inode_number;
        rv = 0;
    }
    free(pair);
    printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
    return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nbtrfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    struct stat st;
    int rv, count;
    int l = count_l(path);
    DirEntry *entries;
    char absolute[PATH_MAX];
    memset(absolute, '\0', PATH_MAX);

    rv = nbtrfs_getattr(path, &st);
    assert(rv == 0);

    filler(buf, ".", &st, 0);
    
    if (l > 0)
    {
        rv = nbtrfs_getattr(parent_path(path, l), &st);
        assert(rv == 0);

        filler(buf, "..", &st, 0);
    }
    
    count = directory_list(disk, cache_s, path, &entries);
    for (int i=0; i<count; i++)
    {
        if (count_l(path) > 0) snprintf(absolute, PATH_MAX, "%s/%s", path, entries[i].name);
        else snprintf(absolute, PATH_MAX, "%s%s", path, entries[i].name);
        rv = nbtrfs_getattr(absolute, &st);
        assert(rv == 0);
        filler(buf, entries[i].name, &st, 0);
    }
    
    printf("readdir(%s) -> %d\n", path, rv);
    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nbtrfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int rv = inode_allocate(disk, cache_s, mode);
    if (-1 == rv) goto print;
    rv = directory_add_entry(disk, cache_s, parent_path(path, count_l(path)), get_name(path), rv, (FileType)(mode & S_IFMT) );
print:
    printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nbtrfs_mkdir(const char *path, mode_t mode)
{
    int rv = nbtrfs_mknod(path, mode | 040000, 0);
    printf("mkdir(%s) -> %d\n", path, rv);
    return rv;
}

int
nbtrfs_unlink(const char *path)
{
    int rv = directory_remove_entry(disk, cache_s, parent_path(path, count_l(path)), get_name(path));
    printf("unlink(%s) -> %d\n", path, rv);
    return rv;
}

int
nbtrfs_link(const char *from, const char *to)
{
    int rv = -1;
    printf("link(%s => %s) -> %d\n", from, to, rv);
	return rv;
}

int
nbtrfs_rmdir(const char *path)
{
    int rv = -1;
    printf("rmdir(%s) -> %d\n", path, rv);
    return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nbtrfs_rename(const char *from, const char *to)
{
    int rv = -1;
    printf("rename(%s => %s) -> %d\n", from, to, rv);
    return rv;
}

int
nbtrfs_chmod(const char *path, mode_t mode)
{
    int rv = -1;
    printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

int
nbtrfs_truncate(const char *path, off_t size)
{
    int rv = -1;
    printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
    return rv;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nbtrfs_open(const char *path, struct fuse_file_info *fi)
{
    int rv = 0;
    printf("open(%s) -> %d\n", path, rv);
    return rv;
}

// Actually read data
int
nbtrfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int rv = 6;
    strcpy(buf, "hello\n");
    printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Actually write data
int
nbtrfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int rv = 0;
    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Update the timestamps on a file or directory.
int
nbtrfs_utimens(const char* path, const struct timespec ts[2])
{
    int rv = -ENOSYS;
    printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
           path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	return rv;
}

// Extended operations
int
nbtrfs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi,
           unsigned int flags, void* data)
{
    int rv = -1;
    printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
    return rv;
}

void nbtrfs_destroy(void *private_data)
{
    
    printf("Unmounting: Syncing data and cleaning up...\n");

    cache_sync(disk, cache_s);
    
    printf("Unmounting: Freeing cache...\n");
    free_cache(cache_s);

    printf("Cleanup complete.\n");
}

void
nbtrfs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nbtrfs_access;
    ops->getattr  = nbtrfs_getattr;
    ops->readdir  = nbtrfs_readdir;
    ops->mknod    = nbtrfs_mknod;
    ops->mkdir    = nbtrfs_mkdir;
    ops->link     = nbtrfs_link;
    ops->unlink   = nbtrfs_unlink;
    ops->rmdir    = nbtrfs_rmdir;
    ops->rename   = nbtrfs_rename;
    ops->chmod    = nbtrfs_chmod;
    ops->truncate = nbtrfs_truncate;
    ops->open	  = nbtrfs_open;
    ops->read     = nbtrfs_read;
    ops->write    = nbtrfs_write;
    ops->utimens  = nbtrfs_utimens;
    ops->ioctl    = nbtrfs_ioctl;
    ops->destroy  = nbtrfs_destroy;
};

struct fuse_operations nbtrfs_ops;


int
main(int argc, char *argv[])
{
    //assert(argc > 2 && argc < 6);
    printf("TODO: mount %s as data file\n", argv[--argc]);
    //storage_init(argv[--argc]);
    disk = disk_open("my.img");
    cache_s = alloc_cache();
    nbtrfs_init_ops(&nbtrfs_ops);
    return fuse_main(argc, argv, &nbtrfs_ops, NULL);
}


/*
int main()
{
    disk = disk_open("my.img");
    cache_s = alloc_cache();
    btree_print(disk, cache_s, 8, 0);
    nbtrfs_mkdir("/hello", 0755);
    btree_print(disk, cache_s, 8, 0);
    InodeBtreePair *pair = item_search(disk, cache_s, "/hello");
    print_pair(pair);
    free(pair);
    nbtrfs_mknod("/hello.txt", 0755, 0);
    pair = item_search(disk, cache_s, "/hello");
    print_pair(pair);
    btree_print(disk, cache_s, 8, 0);
}
*/
