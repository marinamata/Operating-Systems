#include <iostream>
#include <fstream>
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << argv[0] << ": diskImageFile" << endl;
    return 1;
  }


  Disk disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem fs(&disk);

  super_t super;
  fs.readSuperBlock(&super);

// starting with the string Super on a line
  cout << "Super\n";
  //then inode_region_addr followed by a space   //then the inode region address from the super block.
  cout << "inode_region_addr " << super.inode_region_addr << endl;
  //print the string data_region_addr followed by a space //the data region address from the super block, followed by a newline
  cout << "data_region_addr " << super.data_region_addr << endl;
  cout << endl;

//Next it prints the inode and data bitmaps.
  unsigned char inodeBitmap[super.inode_bitmap_len * UFS_BLOCK_SIZE];
  fs.readInodeBitmap(&super, inodeBitmap);
  cout << "Inode bitmap\n";
  for (int i = 0; i < super.inode_bitmap_len * UFS_BLOCK_SIZE; i++) {
    //cout << (unsigned int) bitmap[idx] << " ";
    cout << (unsigned int) inodeBitmap[i] << " ";
  }
  cout << endl << endl;

  unsigned char dataBitmap[super.data_bitmap_len * UFS_BLOCK_SIZE];
  fs.readDataBitmap(&super, dataBitmap);
  cout << "Data bitmap\n";
  for (int i = 0; i < super.data_bitmap_len * UFS_BLOCK_SIZE; i++) {
    //cout << (unsigned int) bitmap[idx] << " ";
    cout << (unsigned int) dataBitmap[i] << " ";
  }
  cout << endl;

  return 0;
}
