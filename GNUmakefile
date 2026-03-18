CFLAGS = -g
RM_FILES = cache_test my.img mkfs.nbtrfs
# Cache files are optimized out of compilation for mkfs & for builds with CACHE_DISABLED macro...
COMMON_FILES = bitmap.c btr.c cache.c disk.c dl.c fl.c gdl.c hash.c lru.c pci.c superblock.c inode.c

UNAME_S := $(shell uname -s)

# Linux-specific flags
ifeq ($(UNAME_S), Linux)
    CFLAGS += -lbsd
endif

# macOS-specific flags
ifeq ($(UNAME_S), Darwin)
    RM_FILES += *.dSYM
endif

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
	rm -rf $(RM_FILES)

open:
	nvim -p *.h *.c

.PHONY: clean open
