#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

void printFileInfo(LocalFileSystem& fs, int inodeNum) {
    inode_t fileInode;
    if (fs.stat(inodeNum, &fileInode)) {
        return;
    }

    ////first print the string File blocks with a newline at the end
    cout << "File blocks" << endl;

int blockCount = static_cast<int>(ceil(static_cast<double>(fileInode.size) / UFS_BLOCK_SIZE));
int numPrintedBlocks = 0;
////then print each of the disk block numbers for the file to standard out, one disk block number per line.
for (int i = 0; i < DIRECT_PTRS && numPrintedBlocks < blockCount; ++i) {
    if (fileInode.direct[i] != 0) {
        cout << fileInode.direct[i] << endl;
        ++numPrintedBlocks;
    }
}
////print an empty line with only a newline.
cout << endl;

//Next, print the string File data with a newline at the end
    cout << "File data" << endl;
    //then print the full contents of the file to standard out.
    char buffer[MAX_FILE_SIZE];
    fs.read(inodeNum, buffer, fileInode.size);
    cout << buffer;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <diskImageFile> <inodeNumber>" << endl;
        return 1;
    }

    string diskImage = argv[1];
    int inodeNumber = atoi(argv[2]);

    Disk diskInstance(diskImage, 4096);
    LocalFileSystem fs(&diskInstance);

    printFileInfo(fs, inodeNumber);

    return 0;
}
