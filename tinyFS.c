#include <stdio.h>
#include <stdlib.h>
#include "tinyFS.h"
#include "libDisk.h"
#include <string.h>

#define TYPE_OFFSET 0
#define MAGIC_NUMBER 0x45
#define MAGIC_NUMBER_OFFSET 1
#define BLOCK_NUMBER_OFFSET 2
//SUPERBLOCK
#define ROOT_INODE_OFFSET 3
#define FREE_BLOCK_LIST_OFFSET 4
//INODE
#define FILE_NAME_OFFSET  3
#define FILE_SIZE_OFFSET  13
#define DATE_BLOCK_NUMBER_OFFSET 15
#define INODE_LIST_OFFSET  16
//FILEXTENT
#define INODE_BLOCK_NUMBER_OFFSET 3
#define NEXT_BLOCK_NUMBER_OFFSET 4
#define TEXT_OFFSET  5
//TYPES
#define SUPERBLOCK_TYPE 1
#define INODE_TYPE 2
#define FILE_EXTENT_TYPE 3
#define FREE_TYPE 4

SuperBlock *superBlock;
INode *rootINode;
fileDescriptor fd;

INode *makeInode(unsigned char blockNum, char *filename, unsigned char data) {
   RequiredInfo requiredInfo;
   INode *iNode = calloc(sizeof(INode), 1);
   
   requiredInfo.type = 2;
   requiredInfo.magicNumber = 0x45;
   requiredInfo.blockNumber = blockNum;
   
   rootINode->required = requiredInfo;
   rootINode->filename = filename;
   rootINode->size = 15;
   rootINode->data = data;
   rootINode->iNodeList = NULL;
   
   return iNode;
}
FreeBlock *makeFreeBlock(int blockNum) {
   RequiredInfo requiredInfo;
   INode *iNode = calloc(sizeof(INode), 1);
   
   requiredInfo.type = 4;
   requiredInfo.magicNumber = 0x45;
   requiredInfo.blockNumber = blockNum;
   
   FreeBlock *freeBlock = calloc(sizeof(FreeBlock), 1);
   
   freeBlock->required = requiredInfo;
   
   return freeBlock;
}

void cleanBlock(int blockNum) {
   char *buffer = calloc(BLOCKSIZE, 1);
   memset(buffer + TYPE_OFFSET, FREE_TYPE, 1);
   memset(buffer + MAGIC_NUMBER_OFFSET, MAGIC_NUMBER, 1);
   memset(buffer + BLOCK_NUMBER_OFFSET, blockNum, 1);
   writeBlock(fd, blockNum, buffer);
}

/* Makes a blank TinyFS file system of size nBytes on the file specified by
‘filename’. This function should use the emulated disk library to open the
specified file, and upon success, format the file to be mountable. This includes
initializing all data to 0x00, setting magic numbers, initializing and writing
the superblock and inodes, etc. Must return a specified success/error code. */

int tfs_mkfs(char *filename, int nBytes) {
   
   fd = openDisk(filename, nBytes);
   
   int i = 0;
   for(i = 0; i < nBytes / BLOCKSIZE; i++) {
      cleanBlock(i);
   }
   
   superBlock = calloc(sizeof(SuperBlock), 1);
   
   rootINode = makeInode(1, "rootNode", 0);
   writeBlock(fd, 1, rootINode);
   
   RequiredInfo requiredInfo;
   requiredInfo.type = 1;
   requiredInfo.magicNumber = 0x45;
   requiredInfo.blockNumber = 0;
   
   superBlock->required = requiredInfo;
   superBlock->rootInode = rootINode;
   
   FreeBlock *freeBlocks = NULL;

      for(i = 2; i <  nBytes / BLOCKSIZE; i++) {
         FreeBlock *fb = makeFreeBlock(i);
         if(freeBlocks == NULL) {
            freeBlocks = fb;
         }
         
         FreeBlock *temp = freeBlocks;         
         while(temp->next) {
            temp = temp->next;
         }
         temp->next = fb;
      }
   
   superBlock->freeBlocks = freeBlocks;
}

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within
‘filename’. tfs_unmount(void) “unmounts” the currently mounted file system.
As part of the mount operation, tfs_mount should verify the file system is
the correct type. Only one file system may be mounted at a time. Use tfs_unmount
to cleanly unmount the currently mounted file system. Must return a specified
success/error code. */

int tfs_mount(char *filename);

int tfs_unmount(void);

/* Opens a file for reading and writing on the currently mounted file system.
Creates a dynamic resource table entry for the file, and returns a file descriptor
(integer) that can be used to reference this file while the filesystem is mounted.
*/

fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system/disk resources,
and removes table entry */

int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s
content, to the file system. Sets the file pointer to 0 (the start of file)
when done. Returns success/error codes. */

int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

/* deletes a file and marks its blocks as free on disk. */

int tfs_deleteFile(fileDescriptor FD);

/* reads one byte from the file and copies it to buffer, using the current file
pointer location and incrementing it by one upon success. If the file pointer is
already at the end of the file then tfs_readByte() should return an error and not
increment the file pointer. */

int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute).
Returns success/error codes.*/

int tfs_seek(fileDescriptor FD, int offset);
