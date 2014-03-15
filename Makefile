all: tinyFsDemo

tinyFsDemo: tinyFSDriver.c TinyFS_errno.h tinyFS.h tinyFS.c libDisk.h libDisk.c
	gcc tinyFSDriver.c TinyFS_errno.h tinyFS.h tinyFS.c libDisk.h libDisk.c -o tinyFsDemo


clean:
	rm -rf *o tinyFsDemo
