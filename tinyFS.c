#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"

#define TYPE_OFFSET 0
#define MAGIC_NUMBER 0x45
#define MAGIC_NUMBER_OFFSET 1
#define BLOCK_NUMBER_OFFSET 2
//SUPERBLOCK
#define ROOT_INODE_OFFSET 3
#define FREE_BLOCK_LIST_OFFSET 4
//INODE
#define FILE_NAME_OFFSET 3
#define FILE_SIZE_OFFSET 13
#define DATE_BLOCK_NUMBER_OFFSET 15
#define INODE_LIST_OFFSET 16
//FILEXTENT
#define INODE_BLOCK_NUMBER_OFFSET 3
#define NEXT_BLOCK_NUMBER_OFFSET 4
#define TEXT_OFFSET 5
//TYPES
#define SUPERBLOCK_TYPE 1
#define INODE_TYPE 2
#define FILE_EXTENT_TYPE 3
#define FREE_TYPE 4

SuperBlock *superBlock;
INode *rootINode;
fileDescriptor disk;
int isMounted = 0;

short fileDescriptorGenerator = 0;
short openFiles[256]; // SET ALL TO ZERO IN MOUNT

int checkMagicNumber(char magicNumber) {
   if(magicNumber != MAGIC_NUMBER) {
         printf("CORRUPTED DATA!!!!!\n");
         return CORRUPTED_DATA_FLAG;
   }
   
   return 0;
}

INode *findInodeRelatingToFile(int fd, INode *currentInode) {
   if(!currentInode) {
      return NULL;
   }
   
   if(fd == currentInode->fileDescriptor) {
      return currentInode;
   }
   
   INode *current = currentInode->iNodeList;
   while(current) {
      INode *found = findInodeRelatingToFile(fd, current);
      if(found) {
         return found;
      }
      current = current->iNodeList;
   }
   
   return NULL;

}

int findCorrectFileExtent(FileExtent *fileExtent, int blockNum, int numBlocksToReadThrough) {
   char buffer[BLOCKSIZE];

   while(numBlocksToReadThrough >= 0) {
      if(readBlock(disk, blockNum, buffer) == - 1) {
         return READ_WRITE_ERROR;
      }
      memcpy(fileExtent, buffer, BLOCKSIZE);
      
      numBlocksToReadThrough--;
      if(fileExtent->nextFileExtent) {
         blockNum = fileExtent->nextFileExtent->required.blockNumber;
      }
   }
}

INode *makeInode(unsigned char blockNum, char *filename, unsigned char data) {
   RequiredInfo requiredInfo;
   INode *iNode = calloc(sizeof(INode), 1);
   
   requiredInfo.type = 2;
   requiredInfo.magicNumber = 0x45;
   requiredInfo.blockNumber = blockNum;
   
   iNode->required = requiredInfo;
   memcpy((iNode->fileName), filename, 8);
   iNode->size = 0;
   iNode->data = data;
   iNode->iNodeList = NULL;
   iNode->fileDescriptor = -1;
   iNode->filePointer = 0;
   
   return iNode;
}
FreeBlock *makeFreeBlock(int blockNum) {
   RequiredInfo requiredInfo;
   
   requiredInfo.type = 4;
   requiredInfo.magicNumber = 0x45;
   requiredInfo.blockNumber = blockNum;
   
   FreeBlock *freeBlock = calloc(sizeof(FreeBlock), 1);
   
   freeBlock->required = requiredInfo;
   
   return freeBlock;
}

int cleanBlock(int blockNum) {
   char *buffer = calloc(BLOCKSIZE, 1);
   memset(buffer + TYPE_OFFSET, FREE_TYPE, 1);
   memset(buffer + MAGIC_NUMBER_OFFSET, MAGIC_NUMBER, 1);
   memset(buffer + BLOCK_NUMBER_OFFSET, blockNum, 1);
   if(writeBlock(disk, blockNum, buffer) == - 1) {
      return READ_WRITE_ERROR;
   }
}

/* Makes a blank TinyFS file system of size nBytes on the file specified by
‘filename’. This function should use the emulated disk library to open the
specified file, and upon success, format the file to be mountable. This includes
initializing all data to 0x00, setting magic numbers, initializing and writing
the superblock and inodes, etc. Must return a specified success/error code. */

int tfs_mkfs(char *filename, int nBytes) {
   
   int nonMountedDisk = openDisk(filename, nBytes);
  
   if(nonMountedDisk < 0) {
      printf("CANT FIND DISK!\n");
      return DISK_ERROR;
   }
   
   int i = 0;
   for(i = 0; i < nBytes / BLOCKSIZE; i++) {
      if(cleanBlock(i) < 0) {
         return READ_WRITE_ERROR;
      }
   }
   
   superBlock = calloc(sizeof(SuperBlock), 1);
   
   rootINode = makeInode(1, "rootNode", 0);
  
   if(writeBlock(nonMountedDisk, 1, rootINode) == - 1) {
      return READ_WRITE_ERROR;
   }
   
   RequiredInfo requiredInfo;
   requiredInfo.type = 1;
   requiredInfo.magicNumber = 0x45;
   requiredInfo.blockNumber = 0;
   
   superBlock->required = requiredInfo;
   superBlock->rootInode = rootINode;
   
   FreeBlock *freeBlocks = NULL;
   int numFreeBlocks = 0;

   for(i = 2; i <  nBytes / BLOCKSIZE; i++) {
      FreeBlock *fb = makeFreeBlock(i);
      if(freeBlocks == NULL) {
         freeBlocks = fb;
      }
      else {
         FreeBlock *temp = freeBlocks;         
         while(temp->next) {
            temp = temp->next;
         }
         temp->next = fb;
         numFreeBlocks++;
      }
   }
   
   superBlock->freeBlocks = freeBlocks;
   superBlock->numberOfFreeBlocks = numFreeBlocks;
 
   if(writeBlock(nonMountedDisk, 0, superBlock) == -1) {
      printf("WRITE ERROR\n");
      return READ_WRITE_ERROR;
   }
   
   return 1;
}

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within
‘filename’. tfs_unmount(void) “unmounts” the currently mounted file system.
As part of the mount operation, tfs_mount should verify the file system is
the correct type. Only one file system may be mounted at a time. Use tfs_unmount
to cleanly unmount the currently mounted file system. Must return a specified
success/error code. */

int tfs_mount(char *filename) {
   if(isMounted) {
      printf("WARNING UNMOUNTING PREVIOUS FILE SYSTEM\n");
   }
   
   int i = 0;
   for(i = 0; i < BLOCKSIZE; i++) {
      openFiles[i] = 0;
   }
   
   tfs_unmount();
   
   char buffer[BLOCKSIZE];
   disk = openDisk(filename, DEFAULT_DISK_SIZE);
   if(disk < 0) {
      return DISK_ERROR;
   }
   readBlock(disk, 0, buffer);
   
   superBlock = (SuperBlock *) buffer;
   
   if(superBlock->required.type != SUPERBLOCK_TYPE 
      || superBlock->required.magicNumber != MAGIC_NUMBER) {
       return DISK_ERROR;
   }
   isMounted = 1;
   
   return 1;
}
//MIGHT WANT TO CLEAR EVERYTHING
int tfs_unmount(void) {

   if(!isMounted) {
      return 1;
   }
   isMounted = 0;
   superBlock = NULL;
   rootINode == NULL;
   
   return 1;
}

/*
* Helper function to find inode with fileName passed in
*/
INode *findInodeRelatingToFileName(char *fileName, INode *currentInode) {
   if(!currentInode) {
      return NULL;
   }
   
   if(!strcmp(fileName, currentInode->fileName)) { // if the file names are the same
      return currentInode;
   }
   
   INode *current = currentInode->iNodeList;
   while(current) {
      INode *found = findInodeRelatingToFileName(fileName, current);
      if(found) {
         return found;
      }
      current = current->iNodeList;
   }
   
   return NULL;

}

/*
* Helper function to create a file
*/
INode *createFile(char *fileName) {
/*   
   //add new iNode to end of list, is it a list or a tree? how are directories being made?
   
   int blockNum = //how to find??
   
   INode *newInode = makeInode(blockNum, fileName, null); //data should be null during creation I believe? also mark as closed
   // make from freeblock, add to inodelist, beginning or end if IM CRAZY
   
   newInode.fileDescriptor = // global fileDescriptorGenerator and then increment that
   
   return INode;
*/
}

/* Opens a file for reading and writing on the currently mounted file system.
Creates a dynamic resource table entry for the file, and returns a file descriptor
(integer) that can be used to reference this file while the filesystem is mounted.
*/

fileDescriptor tfs_openFile(char *name) {
/*
INode *iNode = findInodeRelatingToFileName(name, superBlock->rootInode); // does not address same names, talk to stephen about that

// error message if already opened?

    if(!iNode) {
      iNode = createFile(name); // MAKE A NEW FILE
    }

    time_t ctime = time(NULL);
    iNode->creation = ctime;
    iNode->modification = ctime;
    iNode->access = ctime;
   
    if(checkMagicNumber(iNode->required.magicNumber) < 0) {
       return CORRUPTED_DATA_FLAG;
    }
   
    if(iNode->filePointer >= iNode->size) {
       return OUT_OF_BOUNDS_FLAG;
    }


int blockNum = //iNode->filePointer / (BLOCKSIZE - 6); // how does this work?
    
    iNode->status = 1; // 1 is for open
// how to add r + w? probably enums and another variable in inode

filesOpen[fileDescriptor] = 1;
return iNode->fileDescriptor;
*/
}

/* Closes the file, de-allocates all system/disk resources,
and removes table entry */

int tfs_closeFile(fileDescriptor FD) {
/*   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode);
   if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n");
      return FILE_NOT_FOUND;
   }
   
   if(checkMagicNumber(iNode->required.magicNumber) < 0) {
      return CORRUPTED_DATA_FLAG;
   }
   
   if(iNode->filePointer >= iNode->size) {
      return OUT_OF_BOUNDS_FLAG;
   }
   
   time_t ctime = time(NULL);
   iNode->access = ctime;
   
   // what system/disk resources? make null?

   // do we even need to go into fileExtent? having the iNode may be enough
   FileExtent *fileExtent;
   
   
   // check before, should we throw error if already closed or do nothing? probably do nothing
   
   filesOpen[iNode.fileDescriptor];
   
   // remember to writeBlock here
   return 1;
*/}

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s
content, to the file system. Sets the file pointer to 0 (the start of file)
when done. Returns success/error codes. */

int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {
/*
   // check before if there is enough space to write the file
   // in superblock->NumberOfFreeBlocks

   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode);
   if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n");
      return FILE_NOT_FOUND;
   }
   
   if(checkMagicNumber(iNode->required.magicNumber) < 0) {
      return CORRUPTED_DATA_FLAG;
   }
   
   if(iNode->filePointer >= iNode->size) {
      return OUT_OF_BOUNDS_FLAG;
   }
   
   time_t ctime = time(NULL);
   iNode->modification = ctime;
   iNode->access = ctime;

   if (iNode->status = 0) {
      //throw error because file is closed
   }
   if (iNode->readWriteFlags = cannotWriteToFile) {
      //throw error because file can only be read from
   }

   // do we even need to go into fileExtent? having the iNode may be enough
   FileExtent *fileExtent = //look in inode, create fileExtent if it does not exist, probably will not unless already written to
   
   
   
   
   // need to memcpy(file + sizeofExtraBitsInHeader, buffer, size);
   // if buffersize is too big, need to get another block, set the pointer in the parent block to the new block and keep writing
   
   // if i put everything into the structure, i can just write the structure down
   
   just copy things into filesystem
   
   // need to writeblock after every file io to update disk
   
   // call twice if i need two blocks
   writeBlock(disk, fileExtent->required.blockNumber, fileExtent); // how to access current file in Inode? we have the whole block to write to, must seek to file location, rewrite writeBlock?
   
   return iNode->data;
*/}



/* deletes a file and marks its blocks as free on disk. */

int tfs_deleteFile(fileDescriptor FD) {
/*INode *iNode = findInodeRelatingToFileName(name, superBlock->rootInode); // does not address same names, talk to stephen about that

    if(!iNode) {
//error no inode
    }
   
    if(checkMagicNumber(iNode->required.magicNumber) < 0) {
       return CORRUPTED_DATA_FLAG;
    }
   
    if(iNode->filePointer >= iNode->size) {
       return OUT_OF_BOUNDS_FLAG;
    }


    FileExtent *fileExtent;
    if(findCorrectFileExtent(fileExtent, iNode->data, blockNum) < 0) { // is fileDescripter represented by iNode->data? I'm assuming so.
       return READ_WRITE_ERROR;
    }

// clear blocks here, remember to add numberoffreeblocks back to list and increment number of free blocks

// loop through linked list to reset pointers and then free that inode that i broke off the chain and add to numofFreeBlocks list

return 1; //SUCCESS CODE
*/}

/* reads one byte from the file and copies it to buffer, using the current file
pointer location and incrementing it by one upon success. If the file pointer is
already at the end of the file then tfs_readByte() should return an error and not
increment the file pointer. */

int tfs_readByte(fileDescriptor FD, char *buffer) {
   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode);
   if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n");
      return FILE_NOT_FOUND;
   }
   
   if(checkMagicNumber(iNode->required.magicNumber) < 0) {
      return CORRUPTED_DATA_FLAG;
   }
   
   if(iNode->filePointer >= iNode->size) {
      return OUT_OF_BOUNDS_FLAG;
   }

   int blockNum = iNode->filePointer / (BLOCKSIZE - 6);
   
   if(iNode->filePointer % (BLOCKSIZE - 6) == 0) {
      printf("TRUTH: %d\n", sizeof(RequiredInfo) + sizeof(unsigned short) + sizeof(FileExtent *) == 6);
      blockNum++;
   }
   
   FileExtent *fileExtent;
   if(findCorrectFileExtent(fileExtent, iNode->data, blockNum) < 0) {
      return READ_WRITE_ERROR;
   } 

   memcpy(buffer, fileExtent + 6 + (iNode->filePointer % (BLOCKSIZE - 6)) , 1);
   iNode->filePointer++;
   
   time_t ctime = time(NULL);
   iNode->access = ctime;
   writeBlock(disk, iNode->required.blockNumber, iNode);
   
   return 1;
}

/* change the file pointer location to offset (absolute).
Returns success/error codes.*/

int tfs_seek(fileDescriptor FD, int offset) {
   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode);
   if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n");
      return FILE_NOT_FOUND;
   }
   
   if(checkMagicNumber(iNode->required.magicNumber) < 0) {
      return CORRUPTED_DATA_FLAG;
   }
   
   if(iNode->filePointer >= iNode->size) {
      return OUT_OF_BOUNDS_FLAG;
   }
   
   iNode->filePointer = offset;
   
   time_t ctime = time(NULL);
   iNode->access = ctime;

   writeBlock(disk, iNode->required.blockNumber, iNode);
   return 1;
}

int tfs_readFileInfo(fileDescriptor FD) {
   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode);
   if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n");
      return FILE_NOT_FOUND;
   }
   
   if(checkMagicNumber(iNode->required.magicNumber) < 0) {
      return CORRUPTED_DATA_FLAG;
   }
   
   printf("Creation Time: %s\n", ctime(&(iNode->creation)));

   return 1;
}

int tfs_rename(char *fileName, char* newName) {
   
   INode *iNode = findInodeRelatingToFileName(fileName, superBlock->rootInode);
  
   if(checkMagicNumber(iNode->required.magicNumber)) {
      return CORRUPTED_DATA_FLAG;
   }
 
   if(!iNode) {
      return FILE_NOT_FOUND;
   }
   
   memcpy(iNode->fileName, newName, 8);
   
   time_t ctime = time(NULL);
   iNode->modification = ctime;

   writeBlock(disk, iNode->required.blockNumber, iNode);

   return 1;
}

void printFileAndDirectories(INode *currentInode) {

   if(!currentInode) {
      return;
   }
   
   printf("%s\n", currentInode->fileName);
   
   INode *current = currentInode->iNodeList;
   while(current) {
      printFileAndDirectories(current);
      current = current->iNodeList;
   }
   
   return;

}

int tfs_readdir() {
   

   return 1;
}

int tfs_createDir(char *dirName) {


   return 1;
}

int tfs_removeDir(char *dirName) {


   return 1;
}

int tfs_removeAll(char *dirName) {


   return 1;
}

int tfs_makeRO(char *name) {
   

   return 1;
}

int tfs_makeRW(char *name) {

   return 1 ;
}

int tfs_writeByte(fileDescriptor FD, unsigned int data) {

   return 1;
}
