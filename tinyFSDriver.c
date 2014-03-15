#include<stdio.h>
#include<stdlib.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"
#define TEST_PHASE_ONE 0
#define TEST_MAKE_MOUNT_UNMOUNT 1
#define TEST_OPEN_CLOSE_FILE 1
#define TEST_WRITE_FILE 1
#define TEST_DELETE_FILE 0
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
   testPhaseTwo();


}
int testPhaseTwo() {
   if(TEST_MAKE_MOUNT_UNMOUNT) {
      error = testMakeMountUnmount();
      testForErrors(error);
   }
   if(TEST_OPEN_CLOSE_FILE) {
      testOpenCloseFile();
      testForErrors(error);
   }
   if(TEST_WRITE_FILE) {
      error = testWriteFile();
      testForErrors(error);
   }
   if(TEST_READ_SEEK_BYTE) {
      error = testReadSeek();
      testForErrors(error);
   }
   
   if(TEST_DELETE_FILE) {
      error = testDeleteFile();
      testForErrors(error);
   }
   
   if (TEST_LARGE_THEN_SMALL) { 
	   error = testLargeThenSmall();
	   testForErrors(error);
   }
   
   if (TEST_TOO_MANY_FILES) {
	   error = testTooManyFiles();
	   testForErrors(error);
   }

}

int testReadSeek() {
   char buffer[1];
   printf("READING BYTE FROM FD1");
   error = tfs_readByte(fd1, buffer);
   testForErrors(error);
   printf("BUFFER SHOULD BE A: %c\n", buffer[0]);
   
   printf("READING BYTE FROM FD2");
   error = tfs_readByte(fd2, buffer);
   testForErrors(error);
   printf("BUFFER SHOULD BE B: %c\n", buffer[0]);
    
   printf("READING BYTE FROM FD3");
   error = tfs_readByte(fd3, buffer);
   testForErrors(error);
   printf("BUFFER SHOULD BE K: %c\n", buffer[0]);

   printf("SEEKINGBYTE FROM FD3\n");      
   error = tfs_seek(fd3, 13);
   testForErrors(error);
 
   printf("READING BYTE FROM FD3\n");
   error = tfs_readByte(fd3, buffer);
   testForErrors(error);
   printf("BUFFER SHOULD BE D: %c\n", buffer[0]);
  
   printf("SEEKINGBYTE FROM FD3\n");      
   error = tfs_seek(fd3, 256);
   testForErrors(error);
   
   printf("READING BYTE FROM FD3\n");
   error = tfs_readByte(fd3, buffer);
   testForErrors(error);
   printf("BUFFER SHOULD BE H: %c\n", buffer[0]);

} 

int testDeleteFile() {
   printf("Deleting File1\n");
   error = tfs_deleteFile(fd1);
   testForErrors(error);   
   
   printf("Deleting File2\n");
   error = tfs_deleteFile(fd2);
   testForErrors(error);        
   
   printf("DELETING A FILE NOT IN SYSTEM\n");
   error = tfs_deleteFile(2566);
   testForErrors(error);  
}

int testWriteFile() {
   printf("Opening File1\n");
   fd1 = tfs_openFile("file1");
   testForErrors(fd1);     
      
   printf("Opening File2\n");
   fd2 = tfs_openFile("file2");
   testForErrors(fd2);
   
   printf("Opening File3\n");
   fd3 = tfs_openFile("file3");
   testForErrors(fd3);  

   error = tfs_writeFile(fd1, a, 256);
   testForErrors(error);  
   
   error = tfs_writeFile(fd2, b, 256);
   testForErrors(error);  
   
   error = tfs_writeFile(fd3, c, 513);
   testForErrors(error);  
      
   printf("WRITING TO A FILE NOT IN SYSTEM\n");
   error = tfs_writeFile(2542, b, 256);
   testForErrors(error);  
   if(TEST_DELETE_FILE) {
      testDeleteFile(fd1, fd2);
   }
}

int testTooManyFiles() {
	
	printf("Creating too many files\n");
	char buffer[2];
	
	for (int ndx = 0; ndx < 40; ndx++) {
		memcpy(buffer, &ndx, 2);
		
	    fd1 = tfs_openFile(buffer);
	    testForErrors(fd1); 
	}
	
	for (int ndx = 0; ndx < 40; ndx++) {
		memcpy(buffer, &ndx, 2);
			
	    error = tfs_deleteFile(buffer);
	    testForErrors(error);
	}
	
	printf("Deleting files\n");
	  
}

int testLargeThenSmall() {
	
    printf("Opening File3\n");
    fd3 = tfs_openFile("file3");
    testForErrors(fd3);  

    error = tfs_writeFile(fd1, a, 1024);
    testForErrors(error);  
	
	tfs_displayFragments();
	
    error = tfs_writeFile(fd1, a, 100);
    testForErrors(error);  
	
	tfs_displayFragments();
	
    printf("Deleting File1\n");
    error = tfs_deleteFile(fd1);
    testForErrors(error); 
	
}

int testOpenCloseFile() {
      printf("Opening File1\n");
      fd1 = tfs_openFile("file1");
      testForErrors(fd1);  
      
      printf("Opening File2\n");
      fd2 = tfs_openFile("file2");
      testForErrors(fd2);  
/*      
      printf("Closing File1\n");
      tfs_closeFile(fd1);
      testForErrors(fd1);  
*/      
      printf("Opening File3\n");
      fd3 = tfs_openFile("file3");
      testForErrors(fd3);  
/*      
      printf("Closing File3\n");
      error = tfs_closeFile(fd3);
      testForErrors(error);  
      
      printf("Closing File2\n");
      error = tfs_closeFile(fd2);
      testForErrors(error);  
      printf("Closing File4 NOT A FILE IN OUR SYSTEM\n");
      error = tfs_closeFile(123554);
      testForErrors(error);  
*/
}

int testMakeMountUnmount() {
   printf("Making Disk\n");
   error = tfs_mkfs("defaultDisk", 10240);
   testForErrors(error);  
  /* 
   printf("Mounting Disk\n");
   error = tfs_mount("defaultDisk");
   testForErrors(error);  
   
   printf("UnMounting Disk\n");
   error = tfs_unmount();
   testForErrors(error);  
   
   printf("Mounting Disk\n");
   error = tfs_mount("defaultDisk");
   testForErrors(error);  
   
   printf("Mounting Second Disk While First One Is Mounted\n");
   error = tfs_mount("secondFileDisk");
   testForErrors(error);  
   
   printf("UnMounting Disk\n");
   error = tfs_unmount();
   testForErrors(error); 
   
   printf("Mounting Disk\n");
   error = tfs_mount("defaultDisk");
   testForErrors(error);  
   */
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
