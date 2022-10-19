#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <vector>

//Error code defines
#define ERROR_FILE_DOES_NOT_EXIST -2
#define ERROR_FILE_NAME_INVALID -3
#define ERROR_READ_WRITE -4
#define ERROR_DIRECTORY_NOT_FOUND -5

//Color attribute defines
#define NONE 0;
#define RED 1;
#define BLUE 2;
#define GREEN 3;
#define YELLOW 4;
#define ORANGE 5;

struct File {
  int fileDescriptor;
  int inodeBlknum;
  int seekLocation;
  char mode;
  //add more entries here as appropriate
};

struct FileLock {
  int lockID;
  int inode;
};

class FileSystem {
  DiskManager *myDM;
  PartitionManager *myPM;
  char myfileSystemName;
  int myfileSystemSize;
  int lastLockID;
  int lastFileDescriptor;
  std::vector<File*> openFileTable;
  std::vector<FileLock> lockedFiles;

  
  /* declare other private members here */

  public:
    FileSystem(DiskManager *dm, char fileSystemName);
    int createFile(char *filename, int fnameLen);
    int createDirectory(char *dirname, int dnameLen);
    int lockFile(char *filename, int fnameLen);
    int unlockFile(char *filename, int fnameLen, int lockId);
    int deleteFile(char *filename, int fnameLen);
    int deleteDirectory(char *dirname, int dnameLen);
    int openFile(char *filename, int fnameLen, char mode, int lockId);
    int closeFile(int fileDesc);
    int readFile(int fileDesc, char *data, int len);
    int writeFile(int fileDesc, char *data, int len);
    int appendFile(int fileDesc, char *data, int len);
    int seekFile(int fileDesc, int offset, int flag);
    int truncFile(int fileDesc, int offset, int flag);
    int renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2);
    int renameDirectory(char *dirname1, int dnameLen1, char *dirname2, int dnameLen2);
    int getAttribute(char *filename, int fnameLen, int flag /* ... and other parameters as needed */);
    int setAttribute(char *filename, int fnameLen, int flag, int value /* ... and other parameters as needed */);
    void intToChar(char*,int,int);
    int charToInt(char*,int);
    int findDirectoryInode(char*, int);
    int findFileInode(char*, int);
    int getFileSize(int blknum);
    /* declare other public members here */

};
#endif
