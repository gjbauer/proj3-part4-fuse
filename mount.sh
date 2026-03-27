#!/bin/sh

if [ -d "edk2" ]; then
	sudo umount mnt
else
	mkdir mnt
fi

make clean

make mkfs fuse

./mkfs.nbtrfs

./fuse -s -f mnt my.img
