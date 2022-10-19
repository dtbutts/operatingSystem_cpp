
/* Driver 8*/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "client.h"

using namespace std;

/*
  This driver will test the getAttributes() and setAttributes()
  functions. You need to complete this driver according to the
  attributes you have implemented in your file system, before
  testing your program.
  
  
  Required tests:
  get and set on the fs1 on a file
    and on a file that doesn't exist
    and on a file in a directory in fs1
    and on a file that doesn't exist in a directory in fs1

 fs2, fs3
  on a file both get and set on both fs2 and fs3

  samples are provided below.  Use them and/or make up your own.


*/

int main()
{

  Disk *d = new Disk(300, 64, const_cast<char *>("DISK1"));
  DiskPartition *dp = new DiskPartition[3];

  dp[0].partitionName = 'A';
  dp[0].partitionSize = 100;
  dp[1].partitionName = 'B';
  dp[1].partitionSize = 75;
  dp[2].partitionName = 'C';
  dp[2].partitionSize = 105;

  DiskManager *dm = new DiskManager(d, 3, dp);
  FileSystem *fs1 = new FileSystem(dm, 'A');
  FileSystem *fs2 = new FileSystem(dm, 'B');
  FileSystem *fs3 = new FileSystem(dm, 'C');
  Client *c1 = new Client(fs1);
  Client *c2 = new Client(fs2);
  Client *c3 = new Client(fs3);
  Client *c4 = new Client(fs1);
  Client *c5 = new Client(fs2);



  int r;

  //set/get on file in fs1
  r = c1->myFS->setAttribute(const_cast<char *>("/a"), 2, 0, 1);
  cout << "set COLOR of /a on fs1: " << r << (r == 0? " Correct" : " Fail") << endl;
  r = c4->myFS->setAttribute(const_cast<char *>("/a"), 4, 1, 100);
  cout << "set PRIORITY of /a on fs1: " << r << (r == 0? " Correct" : " Fail") << endl;
  //set/get on file in a directory in fs1
  r = c1->myFS->setAttribute(const_cast<char *>("/e/f"), 4, 0, 1);
  cout << "set COLOR of /e/f on fs1: " << r << (r == 0? " Correct" : " Fail") << endl;
  r = c4->myFS->setAttribute(const_cast<char *>("/e/b"), 4, 1, 100);
  cout << "set PRIORITY of /e/b on fs1: " << r << (r == 0? " Correct" : " Fail") << endl;
  r = c1->myFS->getAttribute(const_cast<char *>("/e/f"), 4, 0);
  cout << "get COLOR of /e/f on fs1: " << r << (r == 1 ? " Correct" : " Fail") << endl;
  r = c4->myFS->getAttribute(const_cast<char *>("/e/b"), 4, 1);
  cout << "get PRIORITY of /e/b on fs1: " << r << (r == 100 ? " Correct" : " Fail") << endl;
  //set/get on file that doesnt exist in fs1
  r = c1->myFS->getAttribute(const_cast<char *>("/p"), 2, 0);  //should failed!
  cout << "get COLOR of /p on fs1: " << r << (r==-4 ? " Correct file does not exist" : " Fail") << endl;
  r = c4->myFS->setAttribute(const_cast<char *>("/p"), 2, 1, 40);  //should failed!
  cout << "set PRIORITY of /p on fs1: " << r << (r==-4 ? " Correct file does not exist" : " Fail") << endl;
  //set/get on a file that doest exist in a directory in fs1
  r = c1->myFS->getAttribute(const_cast<char *>("/e/p"), 4, 0);  //should failed!
  cout << "get COLOR of /e/p on fs1: " << r << (r==-4 ? " Correct file does not exist" : " Fail") << endl;
  r = c4->myFS->setAttribute(const_cast<char *>("/e/p"), 4, 1, 40);  //should failed!
  cout << "set PRIORITY of /e/p on fs1: " << r << (r==-4 ? " Correct file does not exist" : " Fail") << endl;


  //set/get in fs2
  r = c2->myFS->setAttribute(const_cast<char *>("/f"), 2, 1, 42);
  cout << "set PRIORITY of /f on fs2: " << r << (r==-4 ? " Correct file does not exist" : " Fail") << endl;
  r = c5->myFS->setAttribute(const_cast<char *>("/z"), 2, 0, 5);
  cout << "set COLOR of /z on fs2: " << r << (r==0 ? " Correct" : " Fail") << endl;
  r = c2->myFS->getAttribute(const_cast<char *>("/f"), 2, 1);
  cout << "get PRIORITY of /f on fs2: " << r << (r==-4 ? " Correct file does not exist" : " Fail") << endl;
  r = c5->myFS->getAttribute(const_cast<char *>("/z"), 2, 0);
  cout << "get COLOR of /z on fs2: " << r << (r==5 ? " Correct" : " Fail") << endl;

  //set/get in fs3
  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/l"), 10, 0, 3);
  cout << "set COLOR of /o/o/o/a/l on fs3: " << r << (r==0 ? " Correct" : " Fail") << endl;
  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/d"), 10, 1, 420);
  cout << "set PRIORITY of /o/o/o/a/d on fs3: " << r << (r==0 ? " Correct" : " Fail") << endl;
  r = c3->myFS->getAttribute(const_cast<char *>("/o/o/o/a/l"), 10, 0);
  cout << "get COLOR of /o/o/o/a/l on fs3: " << r << (r==3 ? " Correct" : " Fail") << endl;
  r = c3->myFS->getAttribute(const_cast<char *>("o/o/o/a/d"), 10, 1);
  cout << "get PRIORITY of /o/o/o/a/d on fs3: " << r << (r==-3 ? " Correct invalid file name" : " Fail") << endl;

  //set/get in fs2
  r = c2->myFS->setAttribute(const_cast<char *>("/f"), 2, 0, 2);
  cout << "set COLOR of /f on fs2: " << r << (r==-4 ? " Correct" : " Fail") << endl;
  r = c5->myFS->setAttribute(const_cast<char *>("/z"), 2, 1, 1234);
  cout << "set PRIORITY of /z on fs2: " << r << (r==0 ? " Correct file does not exist" : " Fail") << endl;
  r = c2->myFS->getAttribute(const_cast<char *>("/f"), 2, 0);
  cout << "get COLOR of /f on fs2: " << r << (r==-4 ? " Correct file does not exist" : " Fail") << endl;
  r = c5->myFS->getAttribute(const_cast<char *>("/z"), 2, 1);
  cout << "get PRIORITY of /z on fs2: " << r << (r==1234 ? " Correct" : " Fail") << endl;

  //set/get in fs3
  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/l"), 10, 0, 1);
  cout << "set COLOR of /o/o/o/a/l on fs3: " << r << (r==0 ? " Correct" : " Fail") << endl;
  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/d"), 10, 1, 1024);
  cout << "set PRIORITY of /o/o/o/a/d on fs3: " << r << (r==0 ? " Correct" : " Fail") << endl;
  r = c3->myFS->getAttribute(const_cast<char *>("/o/o/o/a/l"), 10, 0);
  cout << "get COLOR of /o/o/o/a/l on fs3: " << r << (r==1 ? " Correct" : " Fail") << endl;
  r = c3->myFS->getAttribute(const_cast<char *>("o/o/o/a/d"), 10, 1);
  cout << "get PRIORITY of /o/o/o/a/d on fs3: " << r << (r==-3 ? " Correct invalid file name" : " Fail") << endl;
  return 0;
}
