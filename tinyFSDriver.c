#include<stdio.h>
#include<stdlib.h>
#include "tinyFS.h"
#define TEST_PHASE_ONE 0
#define TEST_MAKE_MOUNT_UNMOUNT 1
#define TEST_OPEN_CLOSE_FILE 1
#define TEST_WRITE_FILE 1
#define TEST_DELETE_FILE 1
#define TEST_READ_SEEK_BYTE 1

int disk;
char a[256] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
char b[256] = "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
char c[513] = "KBBBBBBBBBBBCDCCCCCCMMMMMMMMOOOOOOOOOWWWWWWWKKKKKKLLLLLLDDDDDDPPPPP$$$$$$(((((D)))))))SSSSSSMMMMMCCCCCCNNNNNN!!!!!!!@@@@@@@@@)))))%%%%%%%%%%%%_______$44444999923000djjjjdooeiiiiisoksdlllaSADKDFOWENOFDFNEOOENNENDKDNODFOJADFDFMOF*)I#RJLDFKLJwofj:JNbnasdkf hiHI;LADF';JD AOPJPIJKI JGWERKJLK WPIGJ WIGJGRJ IGGJ'p wrgij;rgk / gj;/e gjgjorjgiogj jo;rjg rjrg jrg eri gjeirgjie jg;;ljag/nonfoiadgfno;ifyu9p2qr803 v8wLsd SDFJSDLFJSDL:FSDSDSDSSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDSDDDDDDDDDDDDDDDDDDDDDDDDD   ddddddddD";

fileDescriptor fd1;
fileDescriptor fd2;
fileDescriptor fd3;

int main() {

   disk = openDisk("defaultDisk", 10240);
   testPhaseOne();
   testPhaseTwo();


}
int testPhaseTwo() {
   if(TEST_MAKE_MOUNT_UNMOUNT) {
      testMakeMountUnmount();
   }
   if(TEST_OPEN_CLOSE_FILE) {
      testOpenCloseFile();
   }
   if(TEST_WRITE_FILE) {
      testWriteFile();
   }
   if(TEST_READ_SEEK_BYTE) {
      testReadSeek();
   }
   
   if(TEST_DELETE_FILE) {
      testDeleteFile();
   }

}

int testReadSeek() {
      char buffer[1];
      printf("READING BYTE FROM FD1");
      tfs_readByte(fd1, buffer);
      printf("BUFFER SHOULD BE A: %c\n", buffer[1]);
      
      printf("READING BYTE FROM FD2");
      tfs_readByte(fd2, buffer);
      printf("BUFFER SHOULD BE B: %c\n", buffer[1]);
       
      printf("READING BYTE FROM FD3");
      tfs_readByte(fd3, buffer);
      printf("BUFFER SHOULD BE K: %c\n", buffer[1]);

      printf("SEEKINGBYTE FROM FD3\n");      
      tfs_seek(fd3, 13);
     
     printf("READING BYTE FROM FD3\n");
     tfs_readByte(fd3, buffer);
     printf("BUFFER SHOULD BE D: %c\n", buffer[1]);
     
      printf("SEEKINGBYTE FROM FD3\n");      
      tfs_seek(fd3, 256);
     
     printf("READING BYTE FROM FD3\n");
     tfs_readByte(fd3, buffer);
     printf("BUFFER SHOULD BE i: %c\n", buffer[1]);

} 

int testDeleteFile() {
   printf("Deleting File1\n");
   tfs_deleteFile(fd1);
   printf("Deleting File2\n");
   tfs_deleteFile(fd2);
      
   printf("DELETING A FILE NOT IN SYSTEM\n");
   tfs_deleteFile(2566);
}

int testWriteFile() {
   printf("Opening File1\n");
   fd1 = tfs_openFile("file1");
      
   printf("Opening File2\n");
   fd2 = tfs_openFile("file2");
   
   printf("Opening File3\n");
   fd3 = tfs_openFile("file3");

   tfs_writeFile(fd1, a, 256);
   tfs_writeFile(fd2, b, 256);
   tfs_writeFile(fd3, c, 513);
      
   printf("WRITING TO A FILE NOT IN SYSTEM\n");
   tfs_writeFile(2542, b, 256);
   
   if(TEST_DELETE_FILE) {
      testDeleteFile(fd1, fd2);
   }
}

int testOpenCloseFile() {
      printf("Opening File1\n");
      fd1 = tfs_openFile("file1");
      
      printf("Opening File2\n");
      fd2 = tfs_openFile("file2");
      
      printf("Closing File1\n");
      tfs_closeFile(fd1);
      
      printf("Opening File3\n");
      fd3 = tfs_openFile("file3");
      
      printf("Closing File3\n");
      tfs_closeFile(fd3);
      
      printf("Closing File2\n");
      tfs_closeFile(fd2);
      
      printf("Closing File4 NOT A FILE IN OUR SYSTEM\n");
      tfs_closeFile(123554);
}

int testMakeMountUnmount() {
   printf("Making Disk\n");
   tfs_mkfs("defaultDisk", 10240);
   printf("Mounting Disk\n");
   tfs_mount("defaultDisk");
   printf("UnMounting Disk\n");
   tfs_unmount();
   printf("Mounting Disk\n");
   tfs_mount("defaultDisk");
   printf("Mounting Second Disk While First One Is Mounted\n");
   tfs_mount("secondFileDisk");
   tfs_unmount();
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
