#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include <iostream>
using namespace std;


PartitionManager::PartitionManager(DiskManager *dm, char partitionname, int partitionsize)
{
  myDM = dm;
  myPartitionName = partitionname;
  myPartitionSize = myDM->getPartitionSize(myPartitionName);
  bitVector = new BitVector(partitionsize);
  /* If needed, initialize bit vector to keep track of free and allocted
     blocks in this partition */
  char* buffer = new char[getBlockSize()];
  readDiskBlock(0, buffer); //Read in the first disk block
  if(buffer[0] == '#'){ //If first byte of the block is a #, then partition must not be initialized
    bitVector->setBit(0); //Set the first block as allocated
    saveVector(); //save the bitvector to disk
  }else{ //partition is already initialized
    bitVector->setBitVector((unsigned int*) buffer); //read in the bitVector
  }
  delete[] buffer;
}

PartitionManager::~PartitionManager()
{
  delete bitVector;
}

int PartitionManager::saveVector() {
  char* buffer = new char[getBlockSize()];
  for(int i = 0; i < getBlockSize(); i++){
    buffer[i] = '#';
  }
  bitVector->getBitVector((unsigned int*) buffer); //save the bitVector to buffer
  int result = writeDiskBlock(0, buffer); //write buffer to disk
  delete[] buffer;
  return result;
}

/*
 * return blocknum, -1 otherwise
 */
int PartitionManager::getFreeDiskBlock()
{
  for(int i = 0; i < myPartitionSize; i++){ //loop through bitVector
    if(bitVector->testBit(i) == 0){ //check if the block is free
      bitVector->setBit(i); //if block is free, set it to allocated
      if(saveVector() == 0) //save the new bitvector
        return i; //return the newly allocated block
      return -1; //return -1 if we fail to save the bit vector
    }
  }
  return -1; //return -1 if no free blocks exist
}

/*
 * return 0 for sucess, -1 otherwise
 */
int PartitionManager::returnDiskBlock(int blknum)
{
  if(bitVector->testBit(blknum) == 1){ //check if the block is actually allocated
   bitVector->resetBit(blknum); //if block is allocated, then free it
   if(saveVector() == 0){ //save the new bitVector
     char* buffer = new char[getBlockSize()];
     for(int i = 0; i < getBlockSize(); i++)
       buffer[i] = '#';
     if(writeDiskBlock(blknum, buffer) != 0)
       return -1; // write error
     return 0; //return success
   }
   return -1; //return -1 if we fail to save the bit vector
  }
  return -1; //return -1 if the requested block isnt allocated (maybe this should return 0?).
}


int PartitionManager::readDiskBlock(int blknum, char *blkdata)
{
  return myDM->readDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::writeDiskBlock(int blknum, char *blkdata)
{
  return myDM->writeDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::getBlockSize() 
{
  return myDM->getBlockSize();
}
