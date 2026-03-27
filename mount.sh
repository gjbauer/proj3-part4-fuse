#!/bin/sh

if [ -d "mnt" ]; then
	sudo umount mnt
fi

make clean

mkdir mnt

make mkfs fuse

./mkfs.nbtrfs

./fuse -s -f mnt my.img
