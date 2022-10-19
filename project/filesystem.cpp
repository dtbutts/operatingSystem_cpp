#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include <time.h>
#include <math.h>
#include <cstdlib>
#include <iostream>
using namespace std;


FileSystem::FileSystem(DiskManager *dm, char fileSystemName)
{
  myDM = dm; //save disk manager
  myfileSystemName = fileSystemName; //save fs name
  myfileSystemSize = dm->getPartitionSize(fileSystemName); //get fs size
  myPM = new PartitionManager(dm, fileSystemName, myfileSystemSize); //create partition manager
  lastLockID = 0;
  lastFileDescriptor = 0;
  char* buffer = new char[dm->getBlockSize()]; //make buffer
  for(int i = 0; i < dm->getBlockSize(); i++){ //fill with '#'
    buffer[i] = '#';
  }
  myPM->readDiskBlock(1, buffer);//read disk block 1 (should be root dir inode)
  if(buffer[0] == '#'){ //If block is empty, then we must create the root dir inode
    int blknum = myPM->getFreeDiskBlock(); //get free block
    if(blknum != 1) //if block 1 is already allocated, then there is a problem
      cout << "ERROR: root directory inode does not exist, but block is already allocated. Filesystem Corruption?" << endl;
    else{ //otherwise create the inode
      for(int i = 0; i < dm->getBlockSize(); i++){ //populate the buffer
        buffer[i] = ' ';
      }
      myPM->writeDiskBlock(blknum, buffer); //write to disk
    }
  }
  delete[] buffer;
}

int FileSystem::createFile(char *filename, int fnameLen)
{
  if(filename[0] != '/' || !((filename[fnameLen-1] >= 65 && filename[fnameLen-1] <= 90) || (filename[fnameLen-1] >= 97 && filename[fnameLen-1] <= 122))){ //check if file name is invalid
    return -3;
  }
  int parentInode = findDirectoryInode(filename, fnameLen-1); //set parent node to root dir inode
  if(parentInode == ERROR_DIRECTORY_NOT_FOUND) return -4;
  if(parentInode < 0) return parentInode;
  int index = 0;
  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  if(myPM->readDiskBlock(parentInode, buffer) != 0){
    delete[] buffer;
    return -4;
  }
  while(buffer[index] != ' '){ //loop through parent inode until we find an empty space
    if(buffer[index] == filename[fnameLen-1] && buffer[index+5] == 'f'){ //if we find an entry with the same name as the file we are creating, then the file already exists
      return -1;
    }else if(index == 54){ //if we run out of directory definitions,
      if(buffer[60] == ' '){ //create new directory inode, if pointer is empty
        int blk = myPM->getFreeDiskBlock(); //get free block
        if(blk == -1){ //return -2 if we were unable to allocate a block
          delete[] buffer;
          return -2;
        }
        intToChar(buffer, blk, 60); //write the block number to the inode
        if(myPM->writeDiskBlock(parentInode, buffer) != 0){
          delete[] buffer;
          return -4;
        }
        parentInode = blk; //set the parent inode to the new inode
        for(int i = 0; i < myPM->getBlockSize(); i++) //create empty buffer for the new inode
          buffer[i] = ' ';
        index = 0; //set index to 0, so we start at the beginning of the new inode
      }else {//if the pointer is not empty
        parentInode = charToInt(buffer, 60); //get the next inode block number
        for(int i = 0; i < myPM->getBlockSize(); i++)
          buffer[i] = '#';
        if(myPM->readDiskBlock(parentInode, buffer) != 0){ //read in the new inode
          delete[] buffer;
          return -4;
        }
        index = 0; //reset index
      }
    }else{ //if none of the above, then increment index
      index += 6;
    }
  }
  int blknum = myPM->getFreeDiskBlock(); //get a free block for the file inode
  if(blknum == -1){
    delete[] buffer;
    return -2;
  }
  buffer[index] = filename[fnameLen-1]; //enter data into directory inode
  intToChar(buffer, blknum, index+1);
  buffer[index+5] = 'f';
  if(myPM->writeDiskBlock(parentInode, buffer) != 0){ //save directory inode to disk
    delete[] buffer;
    return -4;
  }
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = ' ';
  buffer[0] = filename[fnameLen-1]; //enter data into file inode
  buffer[1] = 'f';
  intToChar(buffer, 0, 2);
  if(myPM->writeDiskBlock(blknum, buffer) != 0){ //save file inode to disk
    delete[] buffer;
    return -4;
  }

  delete[] buffer;
  return 0; //return success
}

int FileSystem::createDirectory(char *dirname, int dnameLen)
{
  if(dirname[0] != '/' || !((dirname[dnameLen-1] >= 65 && dirname[dnameLen-1] <= 90) || (dirname[dnameLen-1] >= 97 && dirname[dnameLen-1] <= 122))){ //check if file name is invalid
    return -3;
  }
  int parentInode = findDirectoryInode(dirname, dnameLen-1);
  if(parentInode == ERROR_DIRECTORY_NOT_FOUND) return -4;
  if(parentInode < 0) return parentInode;
  int index = 0;
  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  if(myPM->readDiskBlock(parentInode, buffer) != 0){
    delete[] buffer;
    return -4;
  }
  while(buffer[index] != ' '){ //loop through parent inode until we find an empty space
    if(buffer[index] == dirname[dnameLen-1] && buffer[index+5] == 'd'){ //if we find an entry with the same name as the dir we are creating, then the dir already exists
      return -1;
    }else if(index == 54){ //if we run out of directory definitions,
      if(buffer[60] == ' '){ //create new directory inode, if pointer is empty
        int blk = myPM->getFreeDiskBlock(); //get free block
        if(blk == -1){ //return -2 if we were unable to allocate a block
          delete[] buffer;
          return -2;
        }
        intToChar(buffer, blk, 60); //write the block number to the inode
        if(myPM->writeDiskBlock(parentInode, buffer) != 0){
          delete[] buffer;
          return -4;
        }
        parentInode = blk; //set the parent inode to the new inode
        for(int i = 0; i < myPM->getBlockSize(); i++) //create empty buffer for the new inode
          buffer[i] = ' ';
        index = 0; //set index to 0, so we start at the beginning of the new inode
      }else {//if the pointer is not empty
        parentInode = charToInt(buffer, 60); //get the next inode block number
        for(int i = 0; i < myPM->getBlockSize(); i++)
          buffer[i] = '#';
        if(myPM->readDiskBlock(parentInode, buffer) != 0){ //read in the new inode
          delete[] buffer;
          return -4;
        }
        index = 0; //reset index
      }
    }else{ //if none of the above, then increment index
      index += 6;
    }
  }
  int blknum = myPM->getFreeDiskBlock(); //get a free block for the dir inode
  if(blknum == -1){
    delete[] buffer;
    return -2;
  }
  buffer[index] = dirname[dnameLen-1]; //enter data into directory inode
  intToChar(buffer, blknum, index+1);
  buffer[index+5] = 'd';
  if(myPM->writeDiskBlock(parentInode, buffer) != 0){ //save directory inode to disk
    delete[] buffer;
    return -4;
  }
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = ' ';
  if(myPM->writeDiskBlock(blknum, buffer) != 0){ //save dir inode to disk
    delete[] buffer;
    return -4;
  }

  delete[] buffer;
  return 0; //return success

}

int FileSystem::lockFile(char *filename, int fnameLen)
{
  int inode = findFileInode(filename, fnameLen);
  if(inode < 0) return inode;
  for(FileLock lock : lockedFiles){
    if(lock.inode == inode)
      return -1; //file is already locked
  }
  for(File* f : openFileTable){
    if(f->inodeBlknum == inode){
      return -3; //file already open
    }
  }
  //if we get to this point then we can lock the file
  int lockid = ++lastLockID;
  lockedFiles.push_back({lockid, inode});
  return lockid;
}

int FileSystem::unlockFile(char *filename, int fnameLen, int lockId)
{
  int inode = findFileInode(filename, fnameLen);
  if(inode < 0) return -2; //if there was an error return -2.
  int index = 0;
  for(FileLock lock : lockedFiles){
    if(lock.inode == inode) {
      if(lock.lockID == lockId){
        lockedFiles.erase(lockedFiles.begin()+index);
        return 0;
      }else {
        return -1;
      }
    }
    index++;
  }
  return -2;
}

int FileSystem::deleteFile(char *filename, int fnameLen)
{
  int inode = findFileInode(filename, fnameLen);
  if(inode == ERROR_FILE_DOES_NOT_EXIST) return -1; //file doesnt exist
  if(inode < 0) return -3; //other error;

  for(FileLock lock : lockedFiles){
    if(lock.inode == inode)
      return -2; //file locked
  }

  for(File* file : openFileTable){
    if(file->inodeBlknum == inode)
      return -2; //file open
  }

  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  if(myPM->readDiskBlock(inode, buffer) != 0){
    delete[] buffer;
    return -3; //read error
  }

  int size = charToInt(buffer, 2);
  int blocks = ceil((float)size / (float)myPM->getBlockSize());
  int indirectBlocks = max(blocks-3, 0);
  int directBlocks = min(blocks, 3);

  for(int i = 0; i < directBlocks; i++){
    int dataInode = charToInt(buffer, 6+(4*i));
    myPM->returnDiskBlock(dataInode);
  }

  if(indirectBlocks > 0){
    int indirectInode = charToInt(buffer, 18);
    for(int i = 0; i < myPM->getBlockSize(); i++)
      buffer[i] = '#';
    if(myPM->readDiskBlock(indirectInode, buffer) != 0){
      delete[] buffer;
      return -3; //read error
    }
    for(int i = 0; i < indirectBlocks; i++){
      int dataInode = charToInt(buffer, (4*i));
      myPM->returnDiskBlock(dataInode);
    }
    myPM->returnDiskBlock(indirectInode);
  }
  myPM->returnDiskBlock(inode);

  inode = findDirectoryInode(filename, fnameLen-1);
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  if(myPM->readDiskBlock(inode, buffer) != 0){
    delete[] buffer;
    return -3; //read error
  }
  int i;
  for(i = 0; i <= 64; i+=6){
    if(i == 60){
      if(buffer[i] == ' '){
        delete[] buffer;
        return -3; //unable to find file entry?
      }
      inode = charToInt(buffer, i);
      for(int j = 0; j < myPM->getBlockSize(); j++)
        buffer[j] = '#';
      if(myPM->readDiskBlock(inode, buffer) != 0){
        delete[] buffer;
        return -3; //read error
      }
      i = -6; //reset to begining of inode
    }else if(buffer[i] == filename[fnameLen-1] && buffer[i+5] == 'f'){
      break;
    }else if(buffer[i] == ' '){
      delete[] buffer;
      return -3; //unable to find file entry?
    }
  }
  for(int j = 0; j < 6; j++)
    buffer[i+j] = ' ';
  for(int j = i+6; j <= 64; j+= 6){
    if(j == 60){
      if(buffer[j] == ' '){
        myPM->writeDiskBlock(inode, buffer);
        break;
      }else{
        int nextInode = charToInt(buffer, j);
        char* buffer2 = new char[myPM->getBlockSize()];
        for(int k = 0; k < myPM->getBlockSize(); k++)
          buffer2[k] = '#';
        myPM->readDiskBlock(nextInode, buffer2);
        if(buffer2[0] == ' '){
          for(int k = 0; k < 4; k++)
            buffer[60+k] = ' ';
          myPM->writeDiskBlock(inode, buffer);
          myPM->returnDiskBlock(nextInode);
          delete[] buffer2;
          break;
        }else{
          for(int k = 0; k < 6; k++){
            buffer[54+k] = buffer2[k];
            buffer2[k] = ' ';
          }
          if(buffer2[6] == ' '){
            for(int k = 0; k < 4; k++)
              buffer[60+k] = ' ';
            myPM->writeDiskBlock(inode, buffer);
            myPM->returnDiskBlock(nextInode);
            delete[] buffer2;
            break;
          }else{
            myPM->writeDiskBlock(inode, buffer);
            inode = nextInode;
            for(int k = 0; k < myPM->getBlockSize(); k++)
              buffer[k] = buffer2[k];
            delete[] buffer2;
            j = 0;
          }
        }
      }
    }else if(buffer[j] == ' '){
      myPM->writeDiskBlock(inode, buffer);
      break;
    }else{
      for(int k = 0; k < 6; k++){
        buffer[j-6+k] = buffer[j+k];
        buffer[j+k] = ' ';
      }
    }
  }
  delete[] buffer;
  return 0;
}


int FileSystem::deleteDirectory(char *dirname, int dnameLen)
{
  char* fullDirName = new char[dnameLen+1];
  for(int i = 0 ; i < dnameLen; i++)
    fullDirName[i] = dirname[i];
  fullDirName[dnameLen] = '/';
  int inode = findDirectoryInode(fullDirName, dnameLen+1);
  if(inode == ERROR_DIRECTORY_NOT_FOUND)
    return -1; //directory not found
  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  myPM->readDiskBlock(inode, buffer);
  if(buffer[0] != ' ')
    return -2; //directory not empty
  delete[] fullDirName;
  myPM->returnDiskBlock(inode);
  inode = findDirectoryInode(dirname, dnameLen-1);
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  myPM->readDiskBlock(inode, buffer);
  int i;
  for(i = 0; i <= 64; i+=6){
    if(i == 60){
      if(buffer[i] == ' '){
        delete[] buffer;
        return -3; //unable to find file entry?
      }
      inode = charToInt(buffer, i);
      for(int j = 0; j < myPM->getBlockSize(); j++)
        buffer[j] = '#';
      if(myPM->readDiskBlock(inode, buffer) != 0){
        delete[] buffer;
        return -3; //read error
      }
      i = -6; //reset to begining of inode
    }else if(buffer[i] == dirname[dnameLen-1] && buffer[i+5] == 'd'){
      break;
    }else if(buffer[i] == ' '){
      delete[] buffer;
      return -3; //unable to find file entry?
    }
  }
  for(int j = 0; j < 6; j++)
    buffer[i+j] = ' ';
  for(int j = i+6; j <= 64; j+= 6){
    if(j == 60){
      if(buffer[j] == ' '){
        myPM->writeDiskBlock(inode, buffer);
        break;
      }else{
        int nextInode = charToInt(buffer, j);
        char* buffer2 = new char[myPM->getBlockSize()];
        for(int k = 0; k < myPM->getBlockSize(); k++)
          buffer2[k] = '#';
        myPM->readDiskBlock(nextInode, buffer2);
        if(buffer2[0] == ' '){
          for(int k = 0; k < 4; k++)
            buffer[60+k] = ' ';
          myPM->writeDiskBlock(inode, buffer);
          myPM->returnDiskBlock(nextInode);
          delete[] buffer2;
          break;
        }else{
          for(int k = 0; k < 6; k++){
            buffer[54+k] = buffer2[k];
            buffer2[k] = ' ';
          }
          if(buffer2[6] == ' '){
            for(int k = 0; k < 4; k++)
              buffer[60+k] = ' ';
            myPM->writeDiskBlock(inode, buffer);
            myPM->returnDiskBlock(nextInode);
            delete[] buffer2;
            break;
          }else{
            myPM->writeDiskBlock(inode, buffer);
            inode = nextInode;
            for(int k = 0; k < myPM->getBlockSize(); k++)
              buffer[k] = buffer2[k];
            delete[] buffer2;
            j = 0;
          }
        }
      }
    }else if(buffer[j] == ' '){
      myPM->writeDiskBlock(inode, buffer);
      break;
    }else{
      for(int k = 0; k < 6; k++){
        buffer[j-6+k] = buffer[j+k];
        buffer[j+k] = ' ';
      }
    }
  }
  delete[] buffer;
  return 0;
}

int FileSystem::openFile(char *filename, int fnameLen, char mode, int lockId)
{
  int inode = findFileInode(filename, fnameLen);
  if(inode == ERROR_READ_WRITE) return -4; //return -4 for misc error
  if(inode == ERROR_FILE_DOES_NOT_EXIST) return -1; // return -1 if file does not exist
  if(mode != 'w' && mode != 'r' && mode != 'm') return -2; //return -2 if mode is invalid
  bool locked = false;
  for(FileLock lock : lockedFiles){
    if(lock.inode == inode){
      locked = true;
      if(lockId == lock.lockID) break;
      else return -3; //file is locked, and lockID doesnt match
    }
  }
  if(!locked && lockId != -1)
    return -3; //file is not locked but lockID was given
  int fileDesc = ++lastFileDescriptor;
  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  if(myPM->readDiskBlock(inode, buffer) != 0){
    delete[] buffer;
    return -4; //general error
  }
  openFileTable.push_back(new File{fileDesc, inode, 0, mode});

  delete[] buffer;
  return fileDesc;
}

int FileSystem::closeFile(int fileDesc)
{
  int index = 0;
  for(File* file : openFileTable){
    if(file->fileDescriptor == fileDesc){
      openFileTable.erase(openFileTable.begin()+index);
      return 0;
    }
    index++;
  }
  return -1;
}

int FileSystem::readFile(int fileDesc, char *data, int len)
{
  //where file inode is read into
  char* inodeBuff = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++){
    inodeBuff[i] = '#';
  }

  //where data is read into
  char* writtenBuff = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++){
    writtenBuff[i] = '#';
  }

  //where indirect inode is read into
  char* indirectInodeBuff = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++){
    indirectInodeBuff[i] = '#';
  }

  //check if its in openFileTable
  for(File* file : openFileTable){
    if(file->fileDescriptor == fileDesc){

      //get file size 
      int fileSize = getFileSize(file->inodeBlknum);
      
      //check if its readable
      if(file->mode == 'w'){
        return -3;  //read operation isn't permitted
      }
      //check if len is valid
      if(len < 0){
        return -2; //length is negative
      }

      //check if cursor is not out of bounds 
      if(file->seekLocation >= fileSize){
        return 0; //nothing read
      }

      int whichBlock = floor(file->seekLocation / 64); //should give which block the seek is at
      int blockIndex = file->seekLocation % 64; //where the seek is in that block
      int toReadIndex = 0;
      int currentBlockAdress;
      int indirectBlock = 0;
      if(whichBlock > 2){        //already in indirect block
        indirectBlock = whichBlock - 3; 
      }
      int charsRead = 0;
      bool endFile = false;



      while(charsRead < len ){ //haven't read full length 

        myPM->readDiskBlock(file->inodeBlknum, inodeBuff);  //read in file inode
        currentBlockAdress = charToInt(inodeBuff, 18); //blknum of indirect inode
        myPM->readDiskBlock(currentBlockAdress, indirectInodeBuff); //read indirect inode into the buffer
        //check if indirect block 
        if(whichBlock < 3){     //it is direct blocks
          currentBlockAdress = charToInt(inodeBuff, 6 + (4 * whichBlock)); //blknum of where we want to read

        }else{  //it is indirect block
          currentBlockAdress = charToInt(indirectInodeBuff, 4 * indirectBlock); //bull the current block from buffer
        }

        myPM->readDiskBlock(currentBlockAdress, writtenBuff);   //read in data block

        while(charsRead < len && blockIndex < 64){

          data[toReadIndex] = writtenBuff[blockIndex];

          toReadIndex++;
          blockIndex++;
          charsRead++;
          file->seekLocation++;

          if(file->seekLocation == fileSize){ //reached end of file
            return charsRead; 
          }
        }

        if(charsRead == len){   //done reading
          return charsRead;

        }else{  //not the end, next block

            whichBlock++; //increase block
            blockIndex = 0; //reset block index
            if(whichBlock > 3){
              indirectBlock++;
            }
          }
      }
    }
  }

  //fileDesc wasn't found
  return -1;

}

int FileSystem::writeFile(int fileDesc, char *data, int len)
{
  //fill an new buffer with #
  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++){
    buffer[i] = '#';
  }

  //check if file is in openFileTable
  for(File* file : openFileTable){
    if(file->fileDescriptor == fileDesc){

      //get file size 
      int fileSize = getFileSize(file->inodeBlknum);

      //check if its writable
      if(file->mode == 'r'){ //is only readable, no write
        //cout << "FILE IS NOT WRITABLE" << endl;
        return -3;
      }

      //check if len is valid
      if(len < 0){
        return -2; //length is negative
      }

      

      //create buffers
      char* newData = new char[myPM->getBlockSize()];
      for(int i = 0; i < myPM->getBlockSize(); i++){
        newData[i] = '#';
      }

      //check file size to see which block we are writing to
      int whichBlock = floor(file->seekLocation / 64); //should give which block the seek is at
      int blockIndex = file->seekLocation % 64; //where the seek is in that block
      int toWriteIndex = 0;  //index for data to be written
      int charsWritten = 0;
      int currentBlockAdress; //the partition block address
      int indirectBlock = 0;
      if(whichBlock > 2){   //if seek is already in indirect block
        indirectBlock = whichBlock - 2; 
      }
      int indirectAdress;


      while(charsWritten < len){    //loop until all data is written

        myPM->readDiskBlock(file->inodeBlknum, buffer);  //read in file inode


        //check if in indirect block 
        if(whichBlock < 3){

        //see if file is new
          // if(charToInt(buffer, 4) == 0) {
          //   currentBlockAdress = myPM->getFreeDiskBlock();  //get free block
          //   intToChar(buffer, currentBlockAdress, 6); //write new blockAdress to inode buffer
          //   myPM->writeDiskBlock(file->inodeBlknum, buffer); //update inode
       
          // }else 
          if(buffer[6 + (4 * whichBlock)] == ' '){   //see if other need new block
            currentBlockAdress = myPM->getFreeDiskBlock();  //get free disk
            if(currentBlockAdress == -1){
              //cout << "FIRST" << endl;
              return -3; //no free disk
            }
            intToChar(buffer, currentBlockAdress, 6 + (4 * whichBlock)); //write new blockAdress to inode buffer
            myPM->writeDiskBlock(file->inodeBlknum, buffer); //update inode
        
          }else{  //already data in the block
            currentBlockAdress = charToInt(buffer, 6 + (4 * whichBlock)); //blknum of where we want to write
          }

          myPM->readDiskBlock(currentBlockAdress, newData);   //pull in old data 
          

        }else{  //it is indirect block

          //see if need to set up indirect block
          if(buffer[18] == ' '){
            currentBlockAdress = myPM->getFreeDiskBlock();  //get free block
            if(currentBlockAdress == -1){
              //cout << "SECOND" << endl;
              return -3; //no free disk
            }
            intToChar(buffer, currentBlockAdress, 18); //write new blockAdress to inode buffer
            myPM->writeDiskBlock(file->inodeBlknum, buffer); //update inode

          }else{  //already has indirect block
            currentBlockAdress = charToInt(buffer, 18); //blknum of indirect inode

          }

          myPM->readDiskBlock(currentBlockAdress, buffer);  //read in indirect inode
          
          indirectAdress = currentBlockAdress;  //save blknum of the indirect node

          //cycle thorough inode directories
          if(buffer[(indirectBlock-1) * 4] == '#') {  //no block for this address
            currentBlockAdress  = myPM->getFreeDiskBlock();
            if(currentBlockAdress == -1){
              //cout << "THIRD" << endl;
              return -3; //no free disk
            }
            intToChar(buffer, currentBlockAdress, (indirectBlock-1) * 4); //update new address to indirect inode
            myPM->writeDiskBlock( indirectAdress, buffer);

          }else{
            currentBlockAdress = charToInt(buffer, 4 * (indirectBlock-1));  //blknum where we want to write
          }

          myPM->readDiskBlock(currentBlockAdress, newData);   //pull in old data


        }

        while(charsWritten < len && blockIndex < 64){   //not end of block, and not done writing

          //check if adding new data, for size update
          if(newData[blockIndex] == '#'){
            fileSize++;  //added new data
          }

          newData[blockIndex] = data[toWriteIndex]; //write data

          blockIndex++; 
          toWriteIndex++;
          charsWritten++;
          file->seekLocation++;
        }

        myPM->writeDiskBlock(currentBlockAdress, newData);  //write block to disk
        myPM->readDiskBlock(file->inodeBlknum, buffer);  //read in file inode
        intToChar(buffer, fileSize, 2);  //update file size
        myPM->writeDiskBlock(file->inodeBlknum, buffer); //save it in file inode

        if(charsWritten == len){  //done writing
          return charsWritten;
       
        }else{  //just ran out of block space
          blockIndex = 0;  //reset
          whichBlock++; //increase block counter
          if(whichBlock > 2){
            indirectBlock++;
          }
        }

      }
    } 
  } 

  //fileDesc wasn't found
  return -1;

}

int FileSystem::appendFile(int fileDesc, char *data, int len)
{

 //check if file is in openFileTable
  for(File* file : openFileTable){
    if(file->fileDescriptor == fileDesc){

      //get file size 
      int fileSize = getFileSize(file->inodeBlknum);

      //check if its writable
      if(file->mode == 'r'){ //is only readable, no write
        return -3;
      }

      //check if len is valid
      if(len < 0){
        return -2; //length is negative
      }

      //check if max file size has or will be reached 
      if((fileSize + len) > 1216){ //max file size is (19 blocks * 64 bytes)
        return -3;  //action not permitted
      }

      //else, append file
      // IS JUST A SEEK TO END OF FILE AND THE WRITE
      seekFile(fileDesc, fileSize , -1);   //seek to first open char at end of file
      return writeFile(fileDesc, data, len);

    }
  }

  //fileDesc wasn't found
  return -1;
}

int FileSystem::truncFile(int fileDesc, int offset, int flag)
{
 int seekReturn;

  //use seek to find the write spot for seekLocation, save the number
  seekReturn = seekFile(fileDesc, offset, flag);

  //return that number if it is an invalid one from seek
  if(seekReturn == -1 || seekReturn == -2 ){
    return seekReturn;
  }

  //see if in open file table
  for(File* file : openFileTable){
    if(file->fileDescriptor == fileDesc){

      //get file size 
      int fileSize = getFileSize(file->inodeBlknum);

      if(fileSize == file->seekLocation){
        return 0; //nothing to delete
      }

      //check if its writable
      if(file->mode == 'r'){ //is only readable, no write
        return -3;
      }

      //create a buffer for block numbers
      char* inodeBuff = new char[64];
      for(int i = 0; i < 64; i++){
        inodeBuff[i] = '#';
      }

      //create a buffer for indirect block
      char* indirectBuff = new char[64];
      for(int i = 0; i < 64; i++){
        indirectBuff[i] = '#';
      }

      int blockNumbers[19]; //to hold file block numbers
      for(int i = 0; i < 19; i++){  
        blockNumbers[i] = -1; 
      }

      //get all blknums of the file
      int i = 0;  //direct block index
      int j = 0;  //indirect block index
      bool done = false;
      myPM->readDiskBlock(file->inodeBlknum, inodeBuff);  //read in file inode
      while( !done ){
        if( i < 3 ){
          //just read direct block and store in blockNumbers[]
          if(inodeBuff[ 6 + ( 4 * i )]  != ' ' ){  //if block is there
            blockNumbers[i] = charToInt(inodeBuff, (6 + ( 4 * i )));  //get directs block numbers
            i++;

          }else{
            done = true;
          }
        
        }else{
          //is indrect block
          if(inodeBuff[18] != ' ' ){ //if indirect is not empty
            myPM->readDiskBlock( charToInt(inodeBuff, 18), indirectBuff);  //read in indirect block

            //now check the indirect block
            if(indirectBuff[ 4 * j ] != '#' && j < 16){ //is address is there
              blockNumbers[i] = charToInt(indirectBuff, (4 * j) );  //add blknum
              i++;
              j++;
              //cout << "IS HITTING INCREMENTS J: " << j << "   AND I: " << i << endl;
              
            }else{
              done = true;
            }

          }else{
            done = true;
          }
        }
        //cout << "GOT A NEW BLOCK: " << i << endl;
      }
      delete[] indirectBuff;

      int charsDeleted = 0;
      int whichBlock = floor(file->seekLocation / 64); //should give which block the seek is at
      int blockIndex = file->seekLocation % 64; //where the seek is in that block
      int numToDelete = fileSize - (file->seekLocation);  //number of characters to trunc
      int firstLineDeletes = 64 - blockIndex;  //number to remove from first line
      int indirectInode;
      int indirectCount;


      // cout << "whichBlock " << whichBlock << endl;
      // cout << "blockIndex " << blockIndex << endl;
      // cout << "numToDelete " << numToDelete << endl;
      // cout << "firstLineDeletes " << firstLineDeletes << endl;

      // bool clearIndirect = false;  //flag for clearing indirect inode if necessary

      char* buffer = new char[64];
      for(int i = 0; i < 64; i++){
        buffer[i] = '#';
      }

      myPM->readDiskBlock(blockNumbers[whichBlock] , buffer);  //read in line to be changed
    
      for(int i = 0; i < firstLineDeletes; i++){  //the whole first line
        if(buffer[blockIndex] != '#'){
          buffer[blockIndex] = '#';  //take out the char
          blockIndex++;
          charsDeleted++;
          fileSize--;
        }
      }
      myPM->writeDiskBlock( blockNumbers[whichBlock] , buffer); //save it in file inode//write this block back to disk
      if(firstLineDeletes == 64){
        myPM->returnDiskBlock(blockNumbers[whichBlock]);  //erase line if seek from zero
        if(whichBlock < 3){ //if direct block
        myPM->readDiskBlock(file->inodeBlknum, inodeBuff);  //read in indirect inode
          for(int i = 0; i < 4; i++){ //erase the current address of direct block
            inodeBuff[6 + (whichBlock * 4) + i] = ' ';
          }
        myPM->writeDiskBlock(file->inodeBlknum, inodeBuff); 
        }
      }

      // get indirect inode block number
      myPM->readDiskBlock(file->inodeBlknum, inodeBuff);  //read in indirect inode
      if(inodeBuff[18] != ' '){
        indirectInode = charToInt(inodeBuff, 18);
        //cout << "THIS IS THE INDIRECT INODE: " << indirectInode << endl;

      }

      if(whichBlock < 1 || (whichBlock == 1 && firstLineDeletes == 64)){
        if(inodeBuff[10] != ' '){ //if there is still a number there
          for(int i = 0; i < 4; i++){
            inodeBuff[10 + i] = ' ';  //empty it
          }
          myPM->writeDiskBlock(file->inodeBlknum, inodeBuff); //write back the new file inode
        }
      }
      if(whichBlock < 2 || (whichBlock == 2 && firstLineDeletes == 64)){
        if(inodeBuff[14] != ' '){ //if there is still a number there
          for(int i = 0; i < 4; i++){
            inodeBuff[14 + i] = ' ';  //empty it
          }
          myPM->writeDiskBlock(file->inodeBlknum, inodeBuff); //write back the new file inode
        }
      }
      if(whichBlock < 3 || (whichBlock == 3 && firstLineDeletes == 64)){
        //get file inode, remove the indirect block number
        if(inodeBuff[18] != ' ' ){ //if indirect is not empty
          int toDeleteIndirect = charToInt(inodeBuff, 18);  //get blk address to wipe
          //cout << "INDIRECT BLOCK NUMBERS: " << toDeleteIndirect << endl;
          for(int i = 0; i < 4; i++){
            inodeBuff[18 + i] = ' ';  //remove indirect block address
          }
          myPM->writeDiskBlock(file->inodeBlknum, inodeBuff); //write back the new file inode
          myPM->returnDiskBlock(toDeleteIndirect);  //remove the indirect inode

        }
      }
      

      delete[] inodeBuff;

      whichBlock++; //increase block
      
      std::vector<int> blocksToWipe;


      //find all other blocks after
      for(int i = whichBlock; i < 19; i++){
        if(blockNumbers[i] != -1){
          // cout << "BLOCKS AFTER THIS ONE "<< blockNumbers[i] << endl;
          blocksToWipe.push_back(blockNumbers[i]);  //get rest of blocks that have data
        }
      }
      indirectCount = blocksToWipe.size(); 


      //returnDiskBlock to all other blocks after the current
      for(int current : blocksToWipe){
        myPM->readDiskBlock(current, buffer);  //read in file inode 
        for(int i = 0; i < 64; i++){
          if(buffer[i] != '#'){
            fileSize--;
            charsDeleted++;
          }
        }
        myPM->returnDiskBlock(current);
      }

      //update new size
      myPM->readDiskBlock(file->inodeBlknum, buffer);  //read in file inode 
      intToChar(buffer, fileSize, 2);  //update file size
      myPM->writeDiskBlock(file->inodeBlknum, buffer); //save it in file inode

      //update indirect block addresses
      char* newBuff = new char[64];
      for(int i = 0; i < 64; i++){
        newBuff[i] = '#';
      }
      //check if there is an indirect block
      myPM->readDiskBlock(file->inodeBlknum, newBuff);  //read in file inode
      //cout << newBuff[18] << endl;
      //cout << newBuff[19] << endl;
      //cout << newBuff[20] << endl;
      //cout << newBuff[21] << endl;
      if(newBuff[18] != ' '){
        indirectInode = charToInt(newBuff, 18); 

        int numOfUsedBlocks = ceil((float)fileSize / 64.0f);  //get total number of blocks used
        int rewriteIndex = (numOfUsedBlocks - 3) * 4; //index of where to start deleting indirect addresses

        if(numOfUsedBlocks > 3){ //i think unneccessary but won't hurt?
          myPM->readDiskBlock(indirectInode, newBuff);  //read in file inode
          for(int i = rewriteIndex; i < 64; i++){
            newBuff[i] = '#';
          }
          myPM->writeDiskBlock(indirectInode, newBuff); //write new indirect inode
        }
      }

      delete[] buffer;
      delete[] newBuff;

      return charsDeleted; //return bytes deleted
    }
  }
  return -1; //file not found
}

int FileSystem::seekFile(int fileDesc, int offset, int flag)
{
  int testOffset;

  //check if file is in openFileTable
  for(File* file : openFileTable){
    if(file->fileDescriptor == fileDesc){

      //get file size 
      int fileSize = getFileSize(file->inodeBlknum);

      //only allow negative offset if flag == 0
      if(offset < 0){ 
        if(flag != 0){
          return -1;    //invalid negative offset
        }
      }

      if(flag == 0){  //offset from current pointer
        //check bounds of offset
        testOffset = file->seekLocation + offset;

        if(testOffset > fileSize || testOffset < 0){    
          return -2;  //tried to go out of file bounds
        }else{
          file->seekLocation = testOffset;  //move it offset bytes forward
          return 0; //success
        }

      }else{  // seek to offset

        if(offset < 0 || offset > fileSize){     
          return -2;  //tried to go out of file bounds
        }else{
          file->seekLocation = offset;
          return 0; //success
        }
      }
    }
  }

  //fileDesc wasn't found
  return -1;

}

int FileSystem::renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2)
{
  int file2inode;
  int file1inode;

  //name validation filename2
  if(filename2[0] != '/' || !((filename2[fnameLen2-1] >= 65 && filename2[fnameLen2-1] <= 90) || (filename2[fnameLen2-1] >= 97 && filename2[fnameLen2-1] <= 122))){ //check if file name is invalid
    return -1;  //invalid new name
  }

  //name validation filename1
  if(filename1[0] != '/' || !((filename1[fnameLen1-1] >= 65 && filename1[fnameLen1-1] <= 90) || (filename1[fnameLen1-1] >= 97 && filename1[fnameLen1-1] <= 122))){ //check if file name is invalid
    return -1;  //invalid old name
  }

  file1inode = findFileInode(filename1, fnameLen1);  //checks if file 1 exists to change
  if(file1inode == -2){
    return -2;  //file does not exist,
  }

  file2inode = findFileInode(filename2, fnameLen2);  //check if it exists
  if(file2inode != -2){
    return -3;  //new filename already exists
  }

  //check if it is open
  for(File* file : openFileTable){
    if(file->inodeBlknum == file1inode){
      return -4; //file is open
    }
  }

  //check if it is locked
  for(FileLock lock : lockedFiles){
    if(lock.inode == file1inode){
      return -4; //file is locked
    }
  }

  
  char* buffer = new char[64];
  for(int i = 0; i < 64; i++){
    buffer[i] = '#';
  }

  //rename file in file inode
  myPM->readDiskBlock(file1inode, buffer);  //read in file inode   
  buffer[0] = filename2[fnameLen2-1]; //enter data into file inode
  myPM->writeDiskBlock(file1inode, buffer); //save it in file inode

  //rename file in directory inode
  int directoryInode = findDirectoryInode(filename1, fnameLen1-1);
  myPM->readDiskBlock(directoryInode, buffer);  //read in file inode
  int i;
  for(i = 0; i <= 64; i+=6){
    if(i == 60){
      if(buffer[i] == ' '){
        delete[] buffer;
        return -3; //unable to find file entry?
      }
      directoryInode = charToInt(buffer, i);
      for(int j = 0; j < myPM->getBlockSize(); j++)
        buffer[j] = '#';
      if(myPM->readDiskBlock(directoryInode, buffer) != 0){
        delete[] buffer;
        return -3; //read error
      }
      i = -6; //reset to begining of inode
    }else if(buffer[i] == filename1[fnameLen1-1] && buffer[i+5] == 'f'){
      break;
    }else if(buffer[i] == ' '){
      delete[] buffer;
      return -3; //unable to find file entry?
    }
  }
  buffer[i] = filename2[fnameLen2-1];
  myPM->writeDiskBlock(directoryInode, buffer); //save it in directory inode

  delete[] buffer;
  return 0; //success
}

int FileSystem::renameDirectory(char *dirname1, int dnameLen1, char *dirname2, int dnameLen2)
{
  // //test if dirname1 is valid
  // if(dirname1[0] != '/' || !((dirname1[dnameLen1-1] >= 65 && dirname1[dnameLen1-1] <= 90) || (dirname1[dnameLen1-1] >= 97 && dirname1[dnameLen1-1] <= 122))){ //check if dir name is invalid
  //   return -1; 
  // }

  // //test if dirname2 is valid
  // if(dirname2[0] != '/' || !((dirname2[dnameLen2-1] >= 65 && dirname2[dnameLen2-1] <= 90) || (dirname2[dnameLen2-1] >= 97 && dirname2[dnameLen2-1] <= 122))){ //check if dir name is invalid
  //   return -1;
  // }

  char* fullDirName1 = new char[dnameLen1+1];
  for(int i = 0 ; i < dnameLen1; i++)
    fullDirName1[i] = dirname1[i];
  fullDirName1[dnameLen1] = '/';
  int dir1inode = findDirectoryInode(fullDirName1, dnameLen1+1);
  if(dir1inode == ERROR_DIRECTORY_NOT_FOUND){
    return -2; //directory not found
  }
  if(dir1inode == -3){
    return -1; //invalid filename
  }


  char* fullDirName2 = new char[dnameLen2+1];
  for(int i = 0 ; i < dnameLen2; i++)
    fullDirName2[i] = dirname2[i];
  fullDirName2[dnameLen2] = '/';
  int dir2inode = findDirectoryInode(fullDirName2, dnameLen2+1);
  if(dir2inode != ERROR_DIRECTORY_NOT_FOUND){
    return -3; //directory not found
  }
  if(dir1inode == -3){
    return -1; //invalid filename
  }

  // //make sure names are valid
  // if(dir1inode == -3 || dir2inode == -3){
  //   return -1; //invalid filename
  // }

  // if(dir1inode == -5){
  //   return -2; //directory doesn't exist
  // }

  // if(dir2inode != -5){
  //   return -3; //directory name is already taken
  // }

  //write new name to directory inode
  char* buffer = new char[64];
  for(int i = 0; i < 64; i++){
    buffer[i] = '#';
  }

  //rename file in directory inode
  int directoryInode = findDirectoryInode(dirname1, dnameLen1-1);
  myPM->readDiskBlock(directoryInode, buffer);  //read in file inode
  int i;
  for(i = 0; i <= 64; i+=6){
    if(i == 60){
      if(buffer[i] == ' '){
        delete[] buffer;
        return -3; //unable to find file entry?
      }
      directoryInode = charToInt(buffer, i);
      for(int j = 0; j < myPM->getBlockSize(); j++)
        buffer[j] = '#';
      if(myPM->readDiskBlock(directoryInode, buffer) != 0){
        delete[] buffer;
        return -3; //read error
      }
      i = -6; //reset to begining of inode
    }else if(buffer[i] == dirname1[dnameLen1-1] && buffer[i+5] == 'd'){
      break;
    }else if(buffer[i] == ' '){
      delete[] buffer;
      return -3; //unable to find file entry?
    }
  }
  buffer[i] = dirname2[dnameLen2-1];
  myPM->writeDiskBlock(directoryInode, buffer); //save it in directory inode
  
  delete[] buffer;
  return 0; //successful
}

/*
 *  The first attribute we have is COLOR. COLOR is stored with a 1 byte int that is defined 
 *  in filesystem.h file. This color can be NONE, 0, or colors 1 - 5. Any inputs outside of 
 *  this range will return -2 as invalid input. If filename is invalid, return -3. If file 
 *  doesn't exist, return -4. If successful, return 0.
 *
 *  COLOR is stored as one byte, at pos = 22.
 *
 *  In getAttribute, input flag decides which attribute to get. If flag = 0, the COLOR 
 *  attribute will be returned. If flag = 1, INSERT MORE HERE. If flag is invalid, return -1.
 *  If invalid file name, return -3. If file doesn't exist, return -4.
 */
int FileSystem::getAttribute(char *filename, int fnameLen, int flag /* ... and other parameters as needed */)
{
  //fill empty buffer
  char* buffer = new char[64];
  for(int i = 0; i < 64; i++){
    buffer[i] = '#';
  }

  //validation

  //name validation of filename
  if(filename[0] != '/' || !((filename[fnameLen-1] >= 65 && filename[fnameLen-1] <= 90) || (filename[fnameLen-1] >= 97 && filename[fnameLen-1] <= 122))){ //check if file name is invalid
    delete[] buffer;
    return -3;  //invalid file name
  }

  int fileInode = findFileInode(filename, fnameLen);  //checks if file exists to change
  if(fileInode == -2){
    delete[] buffer;
    return -4;  //file does not exist,
  }


  if(flag == 0){  //COLOR attribute
    myPM->readDiskBlock(fileInode, buffer);  //read in file inode
    int toReturn = buffer[22];  //grab color
    return toReturn;

  }else if(flag == 1){
    //Priority
    myPM->readDiskBlock(fileInode, buffer);
    int toReturn = charToInt(buffer, 23);
    return toReturn;

  }else{
    return -1; //invalid flag
  }


}

int FileSystem::setAttribute(char *filename, int fnameLen, int flag, int value /* ... and other parameters as needed */)
{
  //fill empty buffer
  char* buffer = new char[64];
  for(int i = 0; i < 64; i++){
    buffer[i] = '#';
  }

  //name validation of filename
  if(filename[0] != '/' || !((filename[fnameLen-1] >= 65 && filename[fnameLen-1] <= 90) || (filename[fnameLen-1] >= 97 && filename[fnameLen-1] <= 122))){ //check if file name is invalid
    delete[] buffer;
    return -3;  //invalid file name
  }

  int fileInode = findFileInode(filename, fnameLen);  //checks if file exists to change
  if(fileInode == -2){
    delete[] buffer;
    return -4;  //file does not exist,
  }
  if(flag == 0){
    //validate input
    if(value < 0 || value > 5){
      delete[] buffer;
      return -2; //invalid input
    }

    int tempColor;
    if(value == 1){
      tempColor = RED;
    }else if(value == 2){
      tempColor = BLUE;
    }else if(value == 3){
      tempColor = GREEN;
    }else if(value == 4){
      tempColor = YELLOW;
    }else if(value == 5){
      tempColor = ORANGE;
    }else{
      tempColor = NONE;
    }

    myPM->readDiskBlock(fileInode, buffer);  //read in file inode
    buffer[22] = tempColor; //put it color
    myPM->writeDiskBlock(fileInode, buffer);
    delete[] buffer;
  }else if(flag == 1){
    if(value < 0 || value > 9999)
      return -2;
    myPM->readDiskBlock(fileInode, buffer);
    intToChar(buffer, value, 23);
    myPM->writeDiskBlock(fileInode, buffer);
    delete[] buffer;
  }else{
    return -1;
  }
  return 0; //successful

}

/*
  convert int to character
*/
void FileSystem::intToChar(char * buffer, int num, int pos){
    string full;

    full = to_string(num);

    int current = pos + 3;
    int numLength = full.length();
    int zeroLength = 4 - numLength;
    int backForward = numLength - 1;

    //where there are actual digits
    for(int i = 0; i < numLength; i++){
        buffer[current] = full.at(backForward);
        current--;
        backForward--;
    }

    //fill in rest with zeros
    for(int i = 0; i < zeroLength; i++){
        buffer[current] = '0';
        current--;
    }
}

/*
  convert character to int
*/
int FileSystem::charToInt(char * buffer, int pos){
    int current = pos;     //location to get char from
    int toReturn;
    char getFour[4];

    //get the correct four chars into a separate array
    for(int i = 0; i < 4; i++){
        getFour[i] = buffer[current];
        current++;
    }

    //turns char to int
    toReturn = atoi(getFour);

    return toReturn;
}


int FileSystem::findDirectoryInode(char* dName, int length) {
  if(dName[0] != '/')
    return ERROR_FILE_NAME_INVALID;
  int inode = 1;
  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  if(myPM->readDiskBlock(inode, buffer) != 0){ //read in root dir inode
    delete[] buffer;
    return ERROR_READ_WRITE;
  }
  for(int i = 1; i <= length-1; i += 2){ //loop through name to traverse file system and find true parent inode
    char name = dName[i];
    if(dName[i+1] != '/' || !((name >= 65 && name <= 90) || (name >= 97 && name <= 122))) //check if name is valid
      return ERROR_FILE_NAME_INVALID;
    int j;
    for(j = 0; j <= 60; j += 6){ //loop through entries in inode
      if(j == 60){
        if(buffer[j] == ' ') //if we exauhsted all entries and there is no pointer to the next inode, then the dir does not exist
          return ERROR_DIRECTORY_NOT_FOUND; //dir not found
        else{
          inode = charToInt(buffer, 60); //if there is a pointer, get the block num
          for(int k = 0; k < myPM->getBlockSize(); k++)
            buffer[k] = '#';
          if(myPM->readDiskBlock(inode, buffer) != 0){ //load the new block into the buffer
            delete[] buffer;
            return ERROR_READ_WRITE;
          }
          j = -6; //reset j so we check from the begining
        }
      }else if(buffer[j] == ' ') //if we run out of directory deffinitions, then the dir does not exist
        return ERROR_DIRECTORY_NOT_FOUND; //dir not found
      else if(buffer[j] == name && buffer[j+5] == 'd') //if we found the dir, break
        break;
    }
    if(buffer[j+5] == 'd'){ //check if it is actually a directory
      inode = charToInt(buffer, j+1); //get the inode block number
      for(int k = 0; k < myPM->getBlockSize(); k++)
        buffer[k] = '#';
      if(myPM->readDiskBlock(inode, buffer) != 0) { //load the inode into buffer
        delete[] buffer;
        return ERROR_READ_WRITE;
      }
    }else{
      delete[] buffer;
      return ERROR_DIRECTORY_NOT_FOUND; //not a directory
    }
  }
  delete[] buffer;
  return inode;

}

int FileSystem::findFileInode(char* name, int length) {
  int inode = findDirectoryInode(name, length-1);
  if(inode == ERROR_DIRECTORY_NOT_FOUND) return ERROR_FILE_DOES_NOT_EXIST; //if dir not found, then file does not exist
  if(inode < 0) return inode; //if any other error happens in findDirectoryInode
  char fname = name[length-1];
  if(!((fname >= 65 && fname <= 90) || (fname >= 97 && fname <= 122))) //check if name is valid
    return ERROR_FILE_NAME_INVALID;
  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++)
    buffer[i] = '#';
  if(myPM->readDiskBlock(inode, buffer) != 0){
    delete[] buffer;
    return ERROR_READ_WRITE;
  }
  int j;
  for(j = 0; j <= 60; j += 6){ //loop through entries in inode
      if(j == 60){
        if(buffer[j] == ' ') //if we exauhsted all entries and there is no pointer to the next inode, then the file does not exist
          return ERROR_FILE_DOES_NOT_EXIST; //file not found
        else{
          inode = charToInt(buffer, 60); //if there is a pointer, get the block num
          for(int k = 0; k < myPM->getBlockSize(); k++)
            buffer[k] = '#';
          if(myPM->readDiskBlock(inode, buffer) != 0){ //load the new block into the buffer
            delete[] buffer;
            return ERROR_READ_WRITE;
          }
          j = -6; //reset j so we check from the begining
        }
      }else if(buffer[j] == ' '){ //if we run out of deffinitions, then the file does not exist
        return ERROR_FILE_DOES_NOT_EXIST; //file not found
      }else if(buffer[j] == fname && buffer[j+5] == 'f') //if we found the file, break
        break;
    }
    if(buffer[j+5] == 'f'){ //check if it is actually a file
      inode = charToInt(buffer, j+1); //get the inode block number
    }else{
      delete[] buffer;
      return ERROR_FILE_DOES_NOT_EXIST; //not a file
    }

  delete[] buffer;
  return inode;
}

/*
    Grabs the file size from the file inode block and 
    Input the block number of the file inode with file->inodeBlknum
*/
int FileSystem::getFileSize(int blknum){

  //create buffer to keep block data
  char* buffer = new char[myPM->getBlockSize()];
  for(int i = 0; i < myPM->getBlockSize(); i++){
    buffer[i] = '#';
  }

  myPM->readDiskBlock(blknum, buffer);  //get inode block

  int size = charToInt(buffer, 2);  //where size is stored

  delete[] buffer;
  return size;
}
