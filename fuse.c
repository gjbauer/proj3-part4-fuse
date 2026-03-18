#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <bsd/string.h>
#include <assert.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "nbtrfs.h"

// implementation for: man 2 access
// Checks if a file exists.
int
nbtrfs_access(const char *path, int mask)
{
    int rv = 0;
    printf("access(%s, %04o) -> %d\n", path, mask, rv);
    return rv;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nbtrfs_getattr(const char *path, struct stat *st)
{
    int rv = 0;
    if (strcmp(path, "/") == 0) {
        st->st_mode = 040755; // directory
        st->st_size = 0;
        st->st_uid = getuid();
    }
    else if (strcmp(path, "/hello.txt") == 0) {
        st->st_mode = 0100644; // regular file
        st->st_size = 6;
        st->st_uid = getuid();
    }
    else {
        rv = -1;
    }
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
    int rv;

    rv = nbtrfs_getattr("/", &st);
    assert(rv == 0);

    filler(buf, ".", &st, 0);

    rv = nbtrfs_getattr("/hello.txt", &st);
    assert(rv == 0);
    filler(buf, "hello.txt", &st, 0);

    printf("readdir(%s) -> %d\n", path, rv);
    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nbtrfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int rv = -1;
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
    int rv = -1;
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
    int rv = -1;
    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Update the timestamps on a file or directory.
int
nbtrfs_utimens(const char* path, const struct timespec ts[2])
{
    int rv = -1;
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
};

struct fuse_operations nbtrfs_ops;

/*int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);
    printf("TODO: mount %s as data file\n", argv[--argc]);
    //storage_init(argv[--argc]);
    nbtrfs_init_ops(&nbtrfs_ops);
    return fuse_main(argc, argv, &nbtrfs_ops, NULL);
}*/

