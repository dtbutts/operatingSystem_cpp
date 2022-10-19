#include "disk.h"
#include "diskmanager.h"
#include <iostream>
using namespace std;

DiskManager::DiskManager(Disk *d, int partcount, DiskPartition *dp)
{
  myDisk = d;               //disk object
  partCount= partcount;     //number of partitions
  int r = myDisk->initDisk();   
  char buffer[64];

  /* If needed, initialize the disk to keep partition information */
  diskP = new DiskPartition[partCount];
  /* else  read back the partition information from the DISK1 */

  // fill the buffer with #, so there is something to test
  for(int i = 0; i < 64; i++){  
    buffer[i] = '#';
  }

  //  read in the superblock to see if the Disk is already created
  myDisk->readDiskBlock(0, buffer); //read in super block

  //if there is a pound, new disk, didn't read in anything
  if(buffer[0] == '#'){
    int index = 0; //index for entering into the buffer
    int startOfPartition = 1;  //gives the block number where partition starts

    for(int i = 0; i < partCount; i++){   //for every partition 
      buffer[index] = dp[i].partitionName;  //store the partitionName 
      index++;  //increase index by 1

      intToChar(buffer, dp[i].partitionSize, index);  //writes four characters to buffer, size of partition
      index += 4; //increase index by 4

      intToChar(buffer, startOfPartition, index); //writes the block where the partition starts, 4 chars
      index += 4; //increase index by 4

      //save info into diskP
      diskP[i].partitionStart = startOfPartition; //save the start of the partition.
      diskP[i].partitionName = dp[i].partitionName; //save name 
      diskP[i].partitionSize = dp[i].partitionSize; //save size

      startOfPartition += dp[i].partitionSize; //update where the next partition will start

    }

    //now write the buffer to the superblock
    myDisk->writeDiskBlock(0, buffer);  //write buffer to block 0

  } else{  //else, already made

    myDisk->readDiskBlock(0, buffer);
    int index = 0;

    //assign diskP fields from the superBlock
    for(int i = 0; i < partCount; i++){
      
      diskP[i].partitionName = buffer[index]; //get the name of the partition
      index++;  //increase 1 character

      diskP[i].partitionSize = charToInt(buffer, index); //get size of partition
      index += 4; //increase 4 characters

      diskP[i].partitionStart = charToInt(buffer, index); //get start of partition
      index +=4;  //increase 4 characters

      // cout << diskP[i].partitionName << endl;    //used for debugging
      // cout << diskP[i].partitionSize << endl;
      // cout << diskP[i].partitionStart << endl;

    }

  }

}

/*
 *   returns: 
 *   0, if the block is successfully read;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds; (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::readDiskBlock(char partitionname, int blknum, char *blkdata)
{
    for( int i = 0; i < partCount; i++){              //test every possible partition
      if(partitionname == diskP[i].partitionName){    //if name matches a name in array of partitions
        return myDisk->readDiskBlock((blknum + diskP[i].partitionStart), blkdata); //will handle the 0, -1, -2 cases
      }
    }

  return -3; //will only reach here if partitionname doesn't exist
}


/*
 *   returns: 
 *   0, if the block is successfully written;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds;  (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::writeDiskBlock(char partitionname, int blknum, char *blkdata)
{
  for(int i = 0; i < partCount; i++){                 //test every possible partition
    if(partitionname == diskP[i].partitionName){      //if name matches a name in array of partitions
      return myDisk->writeDiskBlock((blknum + diskP[i].partitionStart), blkdata); //handles the 0, -1, -2 cases
    }
  }

  return -3; //only reaches here if partitionname doesn't exist
}

/*
 * return size of partition
 * -1 if partition doesn't exist.
 */
int DiskManager::getPartitionSize(char partitionname)
{
  /* write the code for returning partition size */

  for(int i = 0; i < partCount; i++){           //test every possible partition
    if(partitionname == diskP[i].partitionName) //if the partition exists
      return diskP[i].partitionSize;  //returns the size of this partition
  }

  return -1; //no partition has this name
}


/*
  convert character to int
*/
int DiskManager::charToInt(char * buffer, int pos){
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


/*
  convert int to character
*/
void DiskManager::intToChar(char * buffer, int num, int pos){
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