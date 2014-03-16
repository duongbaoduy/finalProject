#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"
#define TEST_PHASE_ONE 0
#define TEST_MAKE_MOUNT_UNMOUNT 1
#define TEST_OPEN_CLOSE_FILE 1
#define TEST_WRITE_FILE 1
#define TEST_DELETE_FILE 1
#define TEST_READ_SEEK_BYTE 1
#define TEST_LARGE_THEN_SMALL 1
#define TEST_TOO_MANY_FILES 1

int disk;
char a[256] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
char b[256] = "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
char c[513] = "KBBBBBBBBBBBCDCCCCCCMMMMMMMMOOOOOOOOOWWWWWWWKKKKKKLLLLLLDDDDDDPPPPP$$$$$$(((((D)))))))SSSSSSMMMMMCCCCCCNNNNNN!!!!!!!@@@@@@@@@)))))%%%%%%%%%%%%_______$44444999923000djjjjdooeiiiiisoksdlllaSADKDFOWENOFDFNEOOENNENDKDNODFOJADFDFMOF*)I#RJLDFKLJwofj:JNbnasdkf hiHI;LADF';JD AOPJPIJKI JGWERKJLK WPIGJ WIGJGRJ IGGJ'p wrgij;rgk / gj;/e gjgjorjgiogj jo;rjg rjrg jrg eri gjeirgjie jg;;ljag/nonfoiadgfno;ifyu9p2qr803 v8wLsd SDFJSDLFJSDL:FSDSDSDSSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDDDDDDDDDDDDDDDDDDDDDDDDD   ddddddddD";

fileDescriptor fd1;
fileDescriptor fd2;
fileDescriptor fd3;

int error = 0;

void testForErrors(int error) {
   if(error == CORRUPTED_DATA_FLAG) {
      printf("CORRUPTED_DATA_FLAG\n");
   }
   else if (error == OUT_OF_BOUNDS_FLAG) {
      printf("OUT_OF_BOUNDS_FLAG\n");
   }
   else if (error == FILE_NOT_FOUND) {
      printf("FILE_NOT_FOUND\n");
   }
   else if (error == DISK_ERROR) {
      printf("DISK_ERROR\n");
   }
   else if (error == READ_WRITE_ERROR) {
      printf("READ_WRITE_ERROR\n");
   }
}

int main() {

   disk = openDisk("defaultDisk", 10240);
   testPhaseOne();
   
   int error = testMakeMountUnmount(); // tests file system consistency checks
   testForErrors(error);
   
   testPhaseTwo();
   
   testTimeStamps(); // tests TimeStamp implementation
   testMakeRO(); // tests ReadOnly implementation
   testDirectoryListingAndRenaming(); // tests Directory Listing and Renaming implementation
   testWriteByte(); // tests writeByte implementation


}
int testPhaseTwo() {
   printf("\n");
   if(TEST_OPEN_CLOSE_FILE) {
      testOpenCloseFile();
   }
   printf("\n");
   if(TEST_WRITE_FILE) {
      error = testWriteFile();
      testForErrors(error);
   }
   printf("\n");   
   if(TEST_READ_SEEK_BYTE) {
      error = testReadSeek();
      testForErrors(error);
   }
   printf("\n");   
   if(TEST_DELETE_FILE) {
      error = testDeleteFile();
      testForErrors(error);
   }
   printf("\n");   
   if (TEST_LARGE_THEN_SMALL) { 
	  error = testLargeThenSmall();
	   testForErrors(error);
   }
   printf("\n");   
   if (TEST_TOO_MANY_FILES) {
	   error = testTooManyFiles();
	   testForErrors(error);
   }

}

int testMakeRO() {
   printf("Opening File1\n");
   fd1 = tfs_openFile("file1");
   testForErrors(fd1);
   
   tfs_writeFile(fd1, a, 256);

   tfs_makeRO("file1");
   if (tfs_writeFile(fd1, a, 256) != READ_WRITE_ERROR) {
      printf("writeFile failed to throw READ_WRITE_ERROR\n");
   }
   if (tfs_writeByte(fd1, 1) != READ_WRITE_ERROR) {
	  printf("writeByte failed to throw READ_WRITE_ERROR\n");
   }
   
   tfs_makeRW("file1");
   int error = tfs_writeFile(fd1, a , 256);
   if (error != 1) {
      printf("WRITEFILE ERROR NUMBER %d\n", error);
   }
   error = tfs_writeByte(fd1, 1);
   if (error != 1) {
      printf("WRITEBYTE ERROR NUMBER %d\n", error);
   }
}

int testTimeStamps() {
   fileDescriptor tempFile;

   printf("Testing TimeStamps...\n");
   printf("Creating tempFile...\n");
   tempFile = tfs_openFile("tempFile");
   printf("Printing Out TimeStamp info for newly created tempFile...\n");
   tfs_readFileInfo(tempFile);
   printf("Writing to tempFile and then printing out updated TimeStamp for tempFile...\n");
   sleep(3);
   tfs_writeFile(tempFile, a, 20);
   tfs_readFileInfo(tempFile);
   tfs_deleteFile(tempFile);
   printf("\n");
}

int testReadSeek() {
   printf("Opening File11...\n");
   fd1 = tfs_openFile("file11");
   testForErrors(fd1);
   
   printf("Opening File22...\n");
   fd2 = tfs_openFile("file22");
   testForErrors(fd2);
   
   printf("Opening File33...\n");
   fd3 = tfs_openFile("file33");
   testForErrors(fd3);
   
   printf("Writing To File11...\n");
   tfs_writeFile(fd1, a, 256);
   
   printf("Writing To File22...\n");
   tfs_writeFile(fd2, b, 256);
   
   printf("Writing To File33...\n");
   tfs_writeFile(fd3, c, 513);
   
   char buffer[1];
   printf("Reading Byte From FD1...\n");
   error = tfs_readByte(fd1, buffer);
   testForErrors(error);
   printf("Buffer Should Be A: %c\n", buffer[0]);
   
   printf("Reading Byte From FD2\n");
   error = tfs_readByte(fd2, buffer);
   testForErrors(error);
   printf("Buffer Should be B: %c\n", buffer[0]);
    
   printf("Reading Byte From FD3...\n");
   error = tfs_readByte(fd3, buffer);
   testForErrors(error);
   printf("Buffer Should Be K: %c\n", buffer[0]);

   printf("Seeking Byte 13 from FD3...\n");      
   error = tfs_seek(fd3, 13);
   testForErrors(error);
 
   printf("Reading Byte From FD3...\n");
   error = tfs_readByte(fd3, buffer);
   testForErrors(error);
   printf("Buffer Should be D: %c\n", buffer[0]);
  
   printf("Seeking Byte 256 from FD3...\n");      
   error = tfs_seek(fd3, 256);
   testForErrors(error);
   
   printf("Reading Byte From FD3...\n");
   error = tfs_readByte(fd3, buffer);
   testForErrors(error);
   printf("Buffer should be H: %c\n", buffer[0]);
} 

int testDeleteFile() {
   printf("Deleting File1...\n");
   error = tfs_deleteFile(fd1);
   testForErrors(error);   
   
   printf("Deleting File2...\n");
   error = tfs_deleteFile(fd2);
   testForErrors(error);        
   
   printf("Deleting A File Not In Our System...\n");
   error = tfs_deleteFile(2566);
   testForErrors(error);  
}


int testWrite(int fd1) {
char buffer[2];
   printf("SEEKINGBYTE FROM FD3\n");      
   error = tfs_seek(fd1, 0);
   testForErrors(error);
   
   printf("WRITING OVER A WITH O\n");
   tfs_writeByte(fd1, 'O');
   
   printf("SEEKINGBYTE FROM FD3\n");      
   error = tfs_seek(fd1, 0);
   testForErrors(error);
   
   printf("READING BYTE FROM FD3\n");
   error = tfs_readByte(fd1, buffer);
   printf("BUFFER SHOULD BE O: %c\n", buffer[0]);
   testForErrors(error);

}

int testDirectoryListingAndRenaming() {
   printf("Printing out files...\n");
   tfs_readdir();
   tfs_openFile("Original");
   printf("Printing out files after adding file named Original...\n");
   tfs_readdir();
   tfs_rename("Original", "Renamed");
   printf("Printing out files after renaming file from Original to Renamed...\n");
   tfs_readdir();
}

int testWriteByte() {
   printf("Testing WriteByte...\n");
   fileDescriptor tempFD = tfs_openFile("WriteByteFile");
   if (tfs_writeByte(tempFD, 'x') != OUT_OF_BOUNDS_FLAG) { // should throw error because fileExtent doesn't exist
      printf("WRITEBYTE TEST FAILED: Did not return out of bounds error");
   }
   tfs_writeFile(tempFD, a, 256);
   tfs_seek(tempFD, 250);
   
   char myString[17] = "THISGetsReplaced";
   int size = 0;
   
   printf("Prints string to be edited by readByte: %s\n", myString);
   tfs_readByte(tempFD, myString + size++);
   tfs_readByte(tempFD, myString + size++);
   tfs_readByte(tempFD, myString + size++);
   tfs_readByte(tempFD, myString + size++);
   printf("Prints string after four readByte calls: %s\n", myString);
   
   printf("Now writing 'Z' bytes into file and reading those bytes into string...\n");
   tfs_seek(tempFD, 250);
   tfs_writeByte(tempFD, 'Z');
   tfs_seek(tempFD, 251);
   tfs_writeByte(tempFD, 'Z');
   tfs_seek(tempFD, 252);
   tfs_writeByte(tempFD, 'Z');
   tfs_seek(tempFD, 253);
   tfs_writeByte(tempFD, 'Z');
   
   tfs_seek(tempFD, 250);
   size= 0;
   tfs_readByte(tempFD, myString + size++);
   tfs_readByte(tempFD, myString + size++);
   tfs_readByte(tempFD, myString + size++);
   tfs_readByte(tempFD, myString + size++);
   
   printf("Should have ZZZZ as first four characters of this string: %s\n", myString);
   
}

int testWriteFile() {
   printf("Opening File1\n");
   fd1 = tfs_openFile("file1");
   testForErrors(fd1);     
   error = tfs_writeFile(fd1, a, 256);   

   error = tfs_writeFile(fd1, a, 256);
   testForErrors(error);  
   
   error = tfs_writeFile(fd2, b, 256);
   testForErrors(error);  
   
   error = tfs_writeFile(fd3, c, 513);
   testForErrors(error);  
      
   printf("Writing To A File Not In Our System...\n");
   error = tfs_writeFile(2542, b, 256);
   testForErrors(error);  
   if(TEST_DELETE_FILE) {
      testDeleteFile(fd1, fd2);
   }
}

int testTooManyFiles() {
	
	printf("Creating too many files...\n");
	char buffer[2];
	int files[40];
	int ndx = 0;
	for (ndx = 0; ndx < 40; ndx++) {
		memcpy(buffer, &ndx, 1);
		buffer[1] = '\0';
		
	    files[ndx] = tfs_openFile(buffer);
	    testForErrors(files[ndx]); 
	}
	
   printf("Deleting files FILE NOT FOUND Means When We Tried To Create The File We Did Not Have Enough Room...\n");
	for (ndx = 0; ndx < 40; ndx++) {	
	      error = tfs_deleteFile(files[ndx]);
	   testForErrors(error);
	}
	  
}

int testLargeThenSmall() {
	
    printf("Creating Large File...\n");
    fd3 = tfs_openFile("file3");
    testForErrors(fd3);  

    error = tfs_writeFile(fd1, a, 1024);
    testForErrors(error);  
	
	tfs_displayFragments();
	
	printf("Creating Small File...\n");
	
	   printf("Opening File11...\n");
   fd1 = tfs_openFile("file11");
   testForErrors(fd1);
   
   printf("Writing To File11...\n");
   tfs_writeFile(fd1, a, 256);
	
   error = tfs_writeFile(fd1, a, 100);
   testForErrors(error);  
	printf("Displaying Fragments S = SuperBlock,  I = Inode, FE = FileExtent, F = Free....\n");
   tfs_displayFragments();
   printf("Defraging FileSystem...\n\n");	
	tfs_defrag();

	printf("Displaying Fragments S = SuperBlock,  I = Inode, FE = FileExtent, F = Free...\n");
	tfs_displayFragments();
	
    printf("Deleting Files\n");
    error = tfs_deleteFile(fd1);
    testForErrors(error); 
	
}

int testOpenCloseFile() {
      printf("Opening File1...\n");
      fd1 = tfs_openFile("file1");
      testForErrors(fd1);  
      
      
      printf("Opening File2...\n");
      fd2 = tfs_openFile("file2");
      testForErrors(fd2);
      
      printf("Opening File3\n");
      fd3 = tfs_openFile("file3");
      testForErrors(fd3); 
      
      printf("Closing 123554 NOT A FILE IN OUR SYSTEM\n");
      error = tfs_closeFile(123554);
      testForErrors(error);  

}

int testMakeMountUnmount() {
   printf("Making Disk...\n");
   error = tfs_mkfs("defaultDisk", 10240);
   testForErrors(error);  
 
   printf("Mounting Disk...\n");
   error = tfs_mount("defaultDisk");
   testForErrors(error);  
   
   printf("UnMounting Disk...\n");
   error = tfs_unmount();
   testForErrors(error);  
   
   printf("Mounting Disk...\n");
   error = tfs_mount("defaultDisk");
   testForErrors(error);  
   printf("Mounting Disk That Doesn't Exist(SHOULD THROW ERROR)...\n");
   error = tfs_mount("secondFileDisk");
   testForErrors(error);  
   
   printf("UnMounting Disk...\n");
   error = tfs_unmount();
   testForErrors(error); 
   
   printf("Mounting Disk...\n");
   error = tfs_mount("defaultDisk");
   testForErrors(error);  
}

int testPhaseOne() {
   if(TEST_PHASE_ONE) {
      writeBlock(disk, 0, a);
      writeBlock(disk, 1, b);
      readBlock(disk, 0, b);
      int i = 0;
      
      for(i = 0; i < 256; i++) {
         printf("%c", b[i]);
      }
      
      writeBlock(disk, 5, a);
   }
}
