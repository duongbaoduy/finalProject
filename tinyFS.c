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

#define FE_SIZE (sizeof(RequiredInfo) + sizeof(unsigned short) + sizeof(FileExtent *))

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
   
   return 1;
}

int checkBlockType(char actualType, char type) {
   return actualType != type ? -1 : 1;
}

int checkBlockNumber(char blockNumber, char correctNumber) {
   if(blockNumber != correctNumber) {
         printf("CORRUPTED DATA!!!!!\n");
         return CORRUPTED_DATA_FLAG;
   }
   
   return 1;
}

INode *findInodeRelatingToFile(int fd, INode *currentInode) {
   if(!currentInode) {
      return NULL;
   }
   
   if(fd == currentInode->fileDescriptor) {
      return currentInode;
   }
   
   INode *current = currentInode->next;
   while(current) {
      INode *found = findInodeRelatingToFile(fd, current);
      if(found) {
         return found;
      }
      current = current->next;
   }
   
   return NULL;

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
   iNode->next = NULL;
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

FileExtent *makeFileExtent(unsigned char blockNum) {
   RequiredInfo requiredInfo;
   FileExtent *fileExtent = calloc(sizeof(FileExtent), 1);
   
   requiredInfo.type = 3; // NOT SURE
   requiredInfo.magicNumber = 0x45; // NOT SURE
   requiredInfo.blockNumber = blockNum;
   
   fileExtent->required = requiredInfo;
   fileExtent->next = NULL;
   
   return fileExtent;
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
      }
      
      numFreeBlocks++;
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
  
   char buffer[BLOCKSIZE];
   int diskToCheck = openDisk(filename, DEFAULT_DISK_SIZE);
   
   if(diskToCheck < 0) {
      return DISK_ERROR;
   }
   readBlock(diskToCheck, 0, buffer);
   
   SuperBlock *superBlockToCheck = (SuperBlock *) buffer;
   
   if(superBlockToCheck->required.type != SUPERBLOCK_TYPE 
      || superBlockToCheck->required.magicNumber != MAGIC_NUMBER) {
       return DISK_ERROR;
   }
   
   if(checkForDiskError(diskToCheck, superBlockToCheck) < 0) {
      printf("FILE SYSTEM IS NOT IN A CONSISTANT STATE\n");
      return DISK_ERROR;
   }
   else {
      int i = 0;
      for(i = 0; i < BLOCKSIZE; i++) {
         openFiles[i] = 0;
      }
      
      superBlock = superBlockToCheck;
      tfs_unmount();
      isMounted = 1;
      disk = diskToCheck;
   }
   return 1;
}
int checkForDiskError(int diskToCheck, SuperBlock *superBlockToCheck) {
   int numberOfFreeBlocks = 0;
   FreeBlock *freeBlock = superBlockToCheck->freeBlocks;
   
   //CHECK NUMBER OF FREE BLOCKS AND TYPE
   while(freeBlock) {
      FileExtent *fileExtent;
      if(checkBlockType(freeBlock->required.type, FREE_TYPE) < 0) {
         return DISK_ERROR;
      }
      freeBlock = freeBlock->next;
      numberOfFreeBlocks++;
   }
   
   if(superBlockToCheck->numberOfFreeBlocks != numberOfFreeBlocks) {
      return DISK_ERROR;
   }
   
   //CHECKING FOR CORRECT MAGIC NUMBERS AND BLOCK NUMBER
   char buffer[BLOCKSIZE];
   int i = 0;
   //THIS IS GOING TO NEED TO BE CHANGED WITH NUMBYTES
   for(i = 0; i < 10240 / BLOCKSIZE; i++) {
      readBlock(diskToCheck, i, buffer);
      if(checkMagicNumber(buffer[MAGIC_NUMBER_OFFSET]) < 0) {
         return DISK_ERROR;
      }
      if(checkBlockNumber(buffer[BLOCK_NUMBER_OFFSET], i) < 0) {
         return DISK_ERROR;
      }
   }
   
   //CHECKING ALL THE INODES FOR CORRECT TYPE AND COUNTING THEM
   int numberOfInodes = checkAllInodes(superBlockToCheck->rootInode);
   if(numberOfInodes < 0) {
      return DISK_ERROR;
   }
   //CHECKING ALL FILE EXTENTS FOR CORRECT TYPE AND COUNTING THEM
   int numberOfFileExtents = checkAllFileExtents(diskToCheck, superBlockToCheck);
   if(numberOfFileExtents < 0) {
      return DISK_ERROR;
   }
   if(numberOfFileExtents + numberOfInodes + numberOfFreeBlocks + 1 != 10240/ BLOCKSIZE) {
      return DISK_ERROR;
   }
   
   return 1;
}

int checkAllFileExtents(int diskToCheck, SuperBlock *superBlock) {
   return countfileExtents(diskToCheck, superBlock->rootInode);;
}

int  countfileExtents(int diskToCheck, INode *currentInode) {
  int numberOfFileExtents = 0;
  if(!currentInode) {
      return 0;
   }
   if(currentInode->next == NULL && currentInode->data > 0) {
      FileExtent *fileExtent;
      readBlock(diskToCheck, currentInode->data, fileExtent);
      numberOfFileExtents++;
      
      while(fileExtent->next) {
      if(checkBlockType(fileExtent->required.type, FILE_EXTENT_TYPE)< 0) {
         return -1;
      }
         numberOfFileExtents++;
         fileExtent = fileExtent->next;
      }
   }
  else {
      INode *current = currentInode->next;
      while(current) {
         int flag = countfileExtents(diskToCheck, current);
         if(flag == DISK_ERROR) {
            return DISK_ERROR;
         }
         
         numberOfFileExtents += flag;
         current = current->next;
      }
   }
   
   return numberOfFileExtents;
}

int checkAllInodes(INode *currentInode) {
   if(!currentInode) {
      return 0;
   }
   if(checkBlockType(currentInode->required.type, INODE_TYPE) < 0) {
      return DISK_ERROR;
   }
   int found  = 0;
   INode *current = currentInode->next;
   while(current) {
      int flag = checkAllInodes(current);
      if(flag == DISK_ERROR) {
         return DISK_ERROR;
      }
      current = current->next;
      found += flag;
   }
   
   return ++found;
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
   //printf("FILENAME: %s\n INODE_NAME: %s\n", fileName, currentInode->fileName);
   if(!strcmp(fileName, currentInode->fileName)) { // if the file names are the same
      return currentInode;
   }
   
   INode *current = currentInode->next;
   while(current) {
      INode *found = findInodeRelatingToFileName(fileName, current);
      if(found) {
         return found;
      }
      current = current->next;
   }
   
   return NULL;

}

/*
 * Helper function to create a file
 */
INode *createFile(char *fileName) {
   
   FreeBlock *freeBlock = superBlock->freeBlocks;
   superBlock->freeBlocks = freeBlock->next; // remove head from freeBlocks list
   superBlock->numberOfFreeBlocks--;
   
   INode *newInode = makeInode(freeBlock->required.blockNumber, fileName, 0); // what is data? change from null to something else
   
   // make from freeblock, add to head of Inode list in superBlock
   newInode->next = superBlock->rootInode;
   superBlock->rootInode = newInode;
   
   newInode->fileDescriptor = fileDescriptorGenerator++;
   
   if(writeBlock(disk, 0, superBlock) == -1) {
      printf("WRITE ERROR\n");
      return NULL;
   }
   
   return newInode;

}

void freeFileExtents(INode *iNode) { //verify with stephen
   FileExtent *fileExtent = iNode->fileExtent;
   
   while (fileExtent != NULL) {
      int tempBlockNum = fileExtent->required.blockNumber; // need this because I clean the block
     FileExtent *temp = fileExtent; // does it make sense to do this for writeBlock?
     fileExtent = fileExtent->next;
     FreeBlock *freeBlock = makeFreeBlock(temp->required.blockNumber);
     
     
      // is cleanBlock even necessary in this? does this even matter because writeBlock is all that affects the disk? do i have to wipe the fileExtent?
     cleanBlock(tempBlockNum); // IS THIS OKAY? SHOULD I LEAVE REQUIRED INFO OR DOES THIS WIPE IT ALL OUT? WHY DOES IT WRITE? WE RESPONSIBLE FOR MEMORY?
      freeBlock->next = superBlock->freeBlocks; // adds freeBlock back into linked list
      superBlock->freeBlocks = freeBlock;
      superBlock->numberOfFreeBlocks++;
     
    // writeBlock(disk, tempBlockNum, temp); // VERIFY WITH STEVEN IF THIS MAKES SENSE AS A 3RD PARAMETER
     
     writeBlock(disk, 0, superBlock);
   }
}

/* Opens a file for reading and writing on the currently mounted file system.
Creates a dynamic resource table entry for the file, and returns a file descriptor
(integer) that can be used to reference this file while the filesystem is mounted.
*/

fileDescriptor tfs_openFile(char *name) {

   INode *iNode = findInodeRelatingToFileName(name, superBlock->rootInode); // does not address same names, talk to stephen about that
   
    if(!iNode) {
      iNode = createFile(name); // MAKE A NEW FILE 
    }
   
    if(iNode == NULL || checkMagicNumber(iNode->required.magicNumber) < 0) {
       return CORRUPTED_DATA_FLAG;
    }
   
    if(iNode-> size > 0 && iNode->filePointer >= iNode->size) {
       return OUT_OF_BOUNDS_FLAG;
    }
   
   openFiles[iNode->fileDescriptor] = 1; // sets file to open
   
   time_t ctime = time(NULL);
    iNode->creation = ctime;
    iNode->modification = ctime;
    iNode->access = ctime;
   
   openFiles[iNode->fileDescriptor] = 1;
   
   writeBlock(disk, iNode->required.blockNumber, iNode);
   return iNode->fileDescriptor; // why not success?
   
}
   

/* Closes the file, de-allocates all system/disk resources,
and removes table entry */

int tfs_closeFile(fileDescriptor FD) {
   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode);
   if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n"); // do we want to printf?
      return FILE_NOT_FOUND;
   }
   
   if(checkMagicNumber(iNode->required.magicNumber) < 0) {
      return CORRUPTED_DATA_FLAG;
   }
   
   time_t ctime = time(NULL);
   iNode->access = ctime;

   openFiles[iNode->fileDescriptor] = 0;
   
   writeBlock(disk, iNode->required.blockNumber, iNode);
   
   return 1;
}
   

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s
content, to the file system. Sets the file pointer to 0 (the start of file)
when done. Returns success/error codes. */

int tfs_writeFile(fileDescriptor FD,char *buffer, int size) { // does writing write a EOF character at end of file? do we need to account for this? 
                                               // what happens to the last two if we write one new block to a file with 3 blocks already?

   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode);
   if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n");
      return FILE_NOT_FOUND;
   }
   
   if (iNode->fileExtent == NULL && superBlock->numberOfFreeBlocks < ((double)size) / BLOCKSIZE) {
      printf("NOT ENOUGH SPACE IN DISK\n");
     return DISK_OUT_OF_SPACE;
   }
   
   if (iNode->fileExtent != NULL) {
       int numOfFileExtents = 1;
      FileExtent *fileEx = iNode->fileExtent;
      while (fileEx != NULL) {
         fileEx = fileEx->next;
        numOfFileExtents++;
      }
      if (superBlock->numberOfFreeBlocks < ((double)size) / BLOCKSIZE - numOfFileExtents) {
         printf("NOT ENOUGH SPACE IN DISK\n");
         return DISK_OUT_OF_SPACE;
      }
   }
   
   if(checkMagicNumber(iNode->required.magicNumber) < 0) {
      return CORRUPTED_DATA_FLAG;
   }
   
   if(iNode->size > 0 && iNode->filePointer > iNode->size) {
      return OUT_OF_BOUNDS_FLAG;
   }
   
   if (openFiles[iNode->fileDescriptor] == 0) {
      return FILE_IS_CLOSED;
   }
/*   
   if (iNode->writeFlag == 0) {
      //throw error because file can only be read from
     return READ_WRITE_ERROR; 
   }
*/   
   time_t ctime = time(NULL);
   iNode->modification = ctime;
   iNode->access = ctime;
   iNode->size = size;
   
   freeFileExtents(iNode); // helper method to clear file before writing
   
   FreeBlock *freeBlock = superBlock->freeBlocks; // create link from INode to fileExtent
   superBlock->freeBlocks = freeBlock->next;
   superBlock->numberOfFreeBlocks--;
   iNode->fileExtent = makeFileExtent(freeBlock->required.blockNumber);
   FileExtent *fileExtent = iNode->fileExtent;
   
   int sizeLeft = size;
   char *bufferSpot = buffer;
   
   while (sizeLeft > BLOCKSIZE - FE_SIZE) {
      FileExtent *tempFileExtent = fileExtent;
     FreeBlock *freeBlock = superBlock->freeBlocks;
      superBlock->freeBlocks = freeBlock->next;
      superBlock->numberOfFreeBlocks--;

     memcpy(fileExtent->data, bufferSpot, BLOCKSIZE - FE_SIZE);
     fileExtent->next = makeFileExtent(freeBlock->required.blockNumber); // set new fileExtent in linked list of previous
     fileExtent = fileExtent->next; // new fileExtent has empty data because we know we need to continue loop and write to it
     
     writeBlock(disk, tempFileExtent->required.blockNumber, tempFileExtent); // writes to block

     bufferSpot += BLOCKSIZE - FE_SIZE; // VERIFY WITH STEPHEN IF I AM INCREMENTING THE POINTER CORRECTLY
     sizeLeft -= BLOCKSIZE - FE_SIZE;
   }
   
   // write rest of bufferSpot
   memcpy(fileExtent->data, bufferSpot, sizeLeft);
   
   writeBlock(disk, iNode->required.blockNumber, iNode); // does it make sense to write Inodes? they aren't full blocksize
   
   if(writeBlock(disk, 0, superBlock) == -1) {
      printf("WRITE ERROR\n");
      return READ_WRITE_ERROR;
   }
   
   return 1;
}

/* deletes a file and marks its blocks as free on disk. */

int tfs_deleteFile(fileDescriptor FD) {
   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode); // does not address same names, talk to stephen about that
   
    if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n");
      return FILE_NOT_FOUND;
   }
   
    if(checkMagicNumber(iNode->required.magicNumber) < 0) {
       return CORRUPTED_DATA_FLAG;
    }
   
   
   freeFileExtents(iNode);
   
   FreeBlock *freeBlock = makeFreeBlock(iNode->required.blockNumber);
   freeBlock->next = superBlock->freeBlocks;
    superBlock->freeBlocks = freeBlock;
    superBlock->numberOfFreeBlocks++;
   
   INode *previousNode;
   if (superBlock->rootInode == iNode) { //does this if statement work?
      superBlock->rootInode = iNode->next; // am I responsible for freeing the Inode i just took off the list? remember to free at end, i still need blockNum
   }
   else {
      previousNode = superBlock->rootInode;
      while(previousNode->next != iNode) {
         previousNode = previousNode->next;
      }
      previousNode->next = iNode->next; // removes iNode from linked list
   }
   
   int tempBlockNum = iNode->required.blockNumber; // maybe not necessary
   
   writeBlock(disk, tempBlockNum, freeBlock); // WHAT DO I WANT HERE? DO I WRITE AN EMPTY BLOCK? JUST REQUIRED INFO? WHAT?
   
   writeBlock(disk, 0, superBlock);
   
   return 1; //SUCCESS CODE
}

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
   
   if(iNode->size > 0 && iNode->filePointer >= iNode->size) {
      return OUT_OF_BOUNDS_FLAG;
   }

   int blockNum = iNode->filePointer / (BLOCKSIZE - FE_SIZE);
   
   if(iNode->filePointer % (BLOCKSIZE - FE_SIZE) == 0 && iNode->filePointer > 0) {
      blockNum++;
   }
   
   FileExtent *fileExtent = iNode->fileExtent;

   while(blockNum > 0) {
      fileExtent = fileExtent->next;
      blockNum--;
   }

   memset(buffer, fileExtent->data[(iNode->filePointer % (BLOCKSIZE - FE_SIZE))], 1);
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
   
   if(iNode->size > 0 && iNode->filePointer >= iNode->size || offset >= iNode->size ) {
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
   
   INode *current = currentInode->next;
   while(current) {
      printFileAndDirectories(current);
      current = current->next;
   }
   
   return;

}

int tfs_readdir() {
   
   if(superBlock->rootInode) {
      printFileAndDirectories(superBlock->rootInode->next);
   }

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

   INode *iNode = findInodeRelatingToFile(FD, superBlock->rootInode);
   if(!iNode) {
      printf("COULDNT FIND THE FILE :(\n");
      return FILE_NOT_FOUND;
   }
   
   if(checkMagicNumber(iNode->required.magicNumber) < 0) {
      return CORRUPTED_DATA_FLAG;
   }
   
   if(iNode->size > 0 && iNode->filePointer >= iNode->size) {
      return OUT_OF_BOUNDS_FLAG;
   }

   int blockNum = iNode->filePointer / (BLOCKSIZE - FE_SIZE);
   
   if(iNode->filePointer % (BLOCKSIZE - FE_SIZE) == 0) {
      blockNum++;
   }
   
   FileExtent *fileExtent = iNode->fileExtent;

   while(blockNum > 0) {
      fileExtent = fileExtent->next;
      blockNum--;
   }

   memset(fileExtent + FE_SIZE + (iNode->filePointer % (BLOCKSIZE - FE_SIZE)), data, 1);
   writeBlock(disk, fileExtent->required.blockNumber, fileExtent);
   iNode->filePointer++;
   
   time_t ctime = time(NULL);
   iNode->access = ctime;
   writeBlock(disk, iNode->required.blockNumber, iNode);

   return 1;
}
