CFLAGS = -I/usr/local/include -L/usr/local/lib -lsysinfo -g
# Cache files are optimized out of compilation for mkfs & for builds with CACHE_DISABLED macro...
COMMON_FILES = bitmap.c btr.c cache.c disk.c dl.c fl.c gdl.c hash.c lru.c pci.c superblock.c inode.c

all:
	clang $(CFLAGS) -o cache_test $(COMMON_FILES) main.c
	dd if=/dev/zero of=my.img bs=1M count=2

sanitize:
	clang $(CFLAGS) -fsanitize=address -O0 -o cache_test $(COMMON_FILES) main.c
	dd if=/dev/zero of=my.img bs=1M count=2

mkfs:
	clang $(CFLAGS) -o mkfs.nbtrfs $(COMMON_FILES) mkfs.c -DCACHE_DISABLED
	dd if=/dev/zero of=my.img bs=1M count=2

clean:
	rm cache_test my.img

open:
	nvim -p *.h *.c

.PHONY: clean open
