#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <cstring>
#include <iomanip> //debug

#include "LocalFileSystem.h"
#include "ufs.h"

#define UFS_BLOCK_SIZE (4096)

using namespace std;


LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {  //filled this out
    char block[UFS_BLOCK_SIZE];
    disk->readBlock(0, block);  // Read the superblock (typically at block 0)
    memcpy(super, block, sizeof(super_t));  // Copy the block data into the superblock structure
}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
   inode_t parentInode;
    if (stat(parentInodeNumber, &parentInode) != 0) {
        return -1;
    }

    if (parentInode.type != UFS_DIRECTORY) {
        return -1;
    }

    int blockNum = parentInode.direct[0];
    if (blockNum == 0) {
        return -1;
    }

    char block[UFS_BLOCK_SIZE];
    disk->readBlock(blockNum, block);

    dir_ent_t* dirEntries = (dir_ent_t*)block;
    for (unsigned long int i = 0; i < UFS_BLOCK_SIZE / sizeof(dir_ent_t); i++) {
        if (dirEntries[i].inum == -1) {
            break;
        }

        if (strncmp(dirEntries[i].name, name.c_str(), DIR_ENT_NAME_SIZE) == 0) {
            return dirEntries[i].inum;
        }
    }

    return -1;
}


int LocalFileSystem::stat(int inodeNumber, inode_t *inode) { //passes all testcases
    super_t super;
    readSuperBlock(&super);

    if (inodeNumber < 0 || inodeNumber >= super.num_inodes) {
        return -EINVALIDINODE;
    }

    int inodeBlockNum = super.inode_region_addr + (inodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE;
    int inodeOffset = (inodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;

    char block[UFS_BLOCK_SIZE];
    disk->readBlock(inodeBlockNum, block);

    //debugging
    //cout << "Reading inode " << inodeNumber << " from block " << inodeBlockNum << " at offset " << inodeOffset << endl;
    //cout << "inode contents: ";

    memcpy(inode, block + inodeOffset, sizeof(inode_t));

    return 0;
}


int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {

    inode_t inode;
    if (stat(inodeNumber, &inode) != 0) {
        return -1;
    }
    if (size < 0 || size > MAX_FILE_SIZE) { //22 hopefully
    return -EINVALIDSIZE;
  }

    if (inodeNumber < 0) {
    return -1;
}
    if (size < 0) {
        return -EINVAL; // Invalid size, should be case 23
    }

    if (inode.type != UFS_REGULAR_FILE && inode.type != UFS_DIRECTORY) {
        return -EINVAL; // Invalid inode type
    }

   

    int bytesRead = 0;
    for (int index = 0; bytesRead < size && bytesRead < inode.size; index++) {
        if (index >= DIRECT_PTRS || inode.direct[index] == 0) {
            break;
        }

        int blockNum = inode.direct[index];

        if (blockNum < 0 || blockNum >= disk->numberOfBlocks()) {
            break;
        }

        char block[UFS_BLOCK_SIZE];
        disk->readBlock(blockNum, block);

        int bytesToRead = min(UFS_BLOCK_SIZE, size - bytesRead);
        memcpy(static_cast<char*>(buffer) + bytesRead, block, bytesToRead);
        bytesRead += bytesToRead;
    }

    return bytesRead;
}
/*(int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
    inode_t parentInode;
   if (name.length() > DIR_ENT_NAME_SIZE){
    return(-EINVALIDNAME);
  }
    if (stat(parentInodeNumber, &parentInode) != 0) {   //yes
        return -EINVALIDINODE;
    }
    if (parentInode.type != UFS_DIRECTORY) {    //testcase
        return -ENOTDIR;
    }
   
    dir_ent_t entries[parentInode.size / sizeof(dir_ent_t) + 1];    //name checks
    read(parentInodeNumber, entries, parentInode.size);
    for (const dir_ent_t& entry : entries) {
        if (strcmp(entry.name, name.c_str()) == 0) {
            if (entry.inum < 0) {
                return -EINVALIDINODE;
            }
            inode_t existingInode;
            int ret = stat(entry.inum, &existingInode);
            if (ret != 0) {
                return ret;
            }
            if (existingInode.type != type) {
                return -EINVALIDTYPE;
            }
            return entry.inum;
        }
    }

    //case 55 added stuff idk: discord: Someone said it has to do with making a directory and initializing . and .. , when u create a directory, you also initialize it with “.” and “..”, you also initialize the regular file, just less complicated
    
    super_t super;
    readSuperBlock(&super);
    unsigned char inodeBitmap[super.inode_bitmap_len * UFS_BLOCK_SIZE];
    readInodeBitmap(&super, inodeBitmap);
    

    int newInodeNumber = -1;
    for (int i = 0; i < super.num_inodes; ++i) {
        if (!(inodeBitmap[i / 8] & (1 << (i % 8)))) {
            inodeBitmap[i / 8] |= (1 << (i % 8));
            newInodeNumber = i;
            break;
        }
    }
    if (newInodeNumber == -1) {
        return -ENOSPC;
    }
    writeInodeBitmap(&super, inodeBitmap);
    
    inode_t newInode;
    newInode.type = type;
    newInode.size = 0;
    memset(newInode.direct, 0, sizeof(newInode.direct));

    //hopefulyy test 55
    if (type == UFS_DIRECTORY) {
        
        dir_ent_t dotEntries[2];
        strcpy(dotEntries[0].name, ".");
        dotEntries[0].inum = newInodeNumber;
        strcpy(dotEntries[1].name, "..");
        dotEntries[1].inum = parentInodeNumber;

        unsigned char dataBitmap[super.data_bitmap_len * UFS_BLOCK_SIZE];
        readDataBitmap(&super, dataBitmap);

        int newBlockNumber = -1;
        for (int i = 0; i < super.num_data; ++i) {
            if (!(dataBitmap[i / 8] & (1 << (i % 8)))) {
                dataBitmap[i / 8] |= (1 << (i % 8));
                newBlockNumber = i;
                break;
            }
        }

        if (newBlockNumber == -1) {
            inodeBitmap[newInodeNumber / 8] &= ~(1 << (newInodeNumber % 8));
            writeInodeBitmap(&super, inodeBitmap);
            return -ENOSPC; // No free data block found
        }
        writeDataBitmap(&super, dataBitmap);

        newInode.direct[0] = super.data_region_addr + newBlockNumber;
        if (write(newInode.direct[0], dotEntries, sizeof(dotEntries)) != sizeof(dotEntries)) {
            inodeBitmap[newInodeNumber / 8] &= ~(1 << (newInodeNumber % 8));
            writeInodeBitmap(&super, inodeBitmap);
            dataBitmap[newBlockNumber / 8] &= ~(1 << (newBlockNumber % 8));
            writeDataBitmap(&super, dataBitmap);
            return -EIO;
        }

        newInode.size = sizeof(dotEntries);
    }

    {
        int inodeBlockNum = super.inode_region_addr + (newInodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE;
        int inodeOffset = (newInodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;

        char block[UFS_BLOCK_SIZE];
        disk->readBlock(inodeBlockNum, block);

        memcpy(block + inodeOffset, &newInode, sizeof(inode_t));

        disk->writeBlock(inodeBlockNum, block);
    }

    dir_ent_t newEntry;
    strcpy(newEntry.name, name.c_str());
    newEntry.inum = newInodeNumber;

    if (write(parentInodeNumber, &newEntry, sizeof(newEntry)) != sizeof(newEntry)) {
        return -EIO;
    }
    parentInode.size += sizeof(newEntry);{
        int parentInodeBlockNum = super.inode_region_addr + (parentInodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE;
        int parentInodeOffset = (parentInodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;

        char block[UFS_BLOCK_SIZE];
        disk->readBlock(parentInodeBlockNum, block);

        memcpy(block + parentInodeOffset, &parentInode, sizeof(inode_t));

        disk->writeBlock(parentInodeBlockNum, block);
    }

    return newInodeNumber;
}*/

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
    inode_t parentInode;

    if (name.length() >= DIR_ENT_NAME_SIZE) {
        return -EINVALIDNAME;
    }

    if (stat(parentInodeNumber, &parentInode) != 0) {
        return -EINVALIDINODE;
    }
    if (parentInode.type != UFS_DIRECTORY) {
        return -ENOTDIR;
    }

    dir_ent_t entries[parentInode.size / sizeof(dir_ent_t) + 1];
    read(parentInodeNumber, entries, parentInode.size);
    for (const dir_ent_t& entry : entries) {
        if (strcmp(entry.name, name.c_str()) == 0) {
            if (entry.inum < 0) {
                return -EINVALIDINODE;
            }
            inode_t existingInode;
            int ret = stat(entry.inum, &existingInode);
            if (ret != 0) {
                return ret;
            }
            if (existingInode.type != type) {
                return -EINVALIDTYPE;
            }
            return entry.inum;
        }
    }

    super_t super;
    readSuperBlock(&super);
    unsigned char inodeBitmap[super.inode_bitmap_len * UFS_BLOCK_SIZE];
    readInodeBitmap(&super, inodeBitmap);

    int newInodeNumber = -1;
    for (int i = 0; i < super.num_inodes; ++i) {
        if (!(inodeBitmap[i / 8] & (1 << (i % 8)))) {
            inodeBitmap[i / 8] |= (1 << (i % 8));
            newInodeNumber = i;
            break;
        }
    }
    if (newInodeNumber == -1) {
        return -ENOSPC;
    }
    writeInodeBitmap(&super, inodeBitmap);

    inode_t newInode;
    newInode.type = type;
    newInode.size = 0;
    memset(newInode.direct, 0, sizeof(newInode.direct));

    if (type == UFS_DIRECTORY) {
        dir_ent_t dotEntries[2];
        strcpy(dotEntries[0].name, ".");
        dotEntries[0].inum = newInodeNumber;
        strcpy(dotEntries[1].name, "..");
        dotEntries[1].inum = parentInodeNumber;

        unsigned char dataBitmap[super.data_bitmap_len * UFS_BLOCK_SIZE];
        readDataBitmap(&super, dataBitmap);

        int newBlockNumber = -1;
        for (int i = 0; i < super.num_data; ++i) {
            if (!(dataBitmap[i / 8] & (1 << (i % 8)))) {
                dataBitmap[i / 8] |= (1 << (i % 8));
                newBlockNumber = i;
                break;
            }
        }

        if (newBlockNumber == -1) {
            inodeBitmap[newInodeNumber / 8] &= ~(1 << (newInodeNumber % 8));
            writeInodeBitmap(&super, inodeBitmap);
            return -ENOSPC;
        }
        writeDataBitmap(&super, dataBitmap);

        newInode.direct[0] = super.data_region_addr + newBlockNumber;
        if (write(newInode.direct[0], dotEntries, sizeof(dotEntries)) != sizeof(dotEntries)) {
            inodeBitmap[newInodeNumber / 8] &= ~(1 << (newInodeNumber % 8));
            writeInodeBitmap(&super, inodeBitmap);
            dataBitmap[newBlockNumber / 8] &= ~(1 << (newBlockNumber % 8));
            writeDataBitmap(&super, dataBitmap);
            return -EIO;
        }

        newInode.size = sizeof(dotEntries);
    }{
        int inodeBlockNum = super.inode_region_addr + (newInodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE;
        int inodeOffset = (newInodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;

        char block[UFS_BLOCK_SIZE];
        disk->readBlock(inodeBlockNum, block);

        memcpy(block + inodeOffset, &newInode, sizeof(inode_t));

        disk->writeBlock(inodeBlockNum, block);
    }

    dir_ent_t newEntry;
    strcpy(newEntry.name, name.c_str());
    newEntry.inum = newInodeNumber;

    if (write(parentInodeNumber, &newEntry, sizeof(newEntry)) != sizeof(newEntry)) {
        return -EIO;
    }

    parentInode.size += sizeof(newEntry);
    {
        int parentInodeBlockNum = super.inode_region_addr + (parentInodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE;
        int parentInodeOffset = (parentInodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;

        char block[UFS_BLOCK_SIZE];
        disk->readBlock(parentInodeBlockNum, block);

        memcpy(block + parentInodeOffset, &parentInode, sizeof(inode_t));

        disk->writeBlock(parentInodeBlockNum, block);
    }

    return newInodeNumber;
}


/*int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
    if (size <= 0) {
        return -EINVAL;
    }
    inode_t inode;
    super_t super;
    if (stat(inodeNumber, &inode) != 0) {
        return -1;
    }

    if (buffer == nullptr) {
        return -EFAULT;
    }

    if (inodeNumber < 0) {
        return -1;
    }

    //case 43 fingers crossed
    if (inodeNumber < 0 || inodeNumber == super.num_inodes) {
        return -EINVALIDINODE;
    }

    if (inode.type != UFS_REGULAR_FILE) {
    return EINVALIDTYPE;
  }
   if (size < 0 || size > MAX_FILE_SIZE) {
    return EINVALIDSIZE;
  }

    readSuperBlock(&super);
    unsigned char dataBitmap[super.data_bitmap_len * UFS_BLOCK_SIZE];
    readDataBitmap(&super, dataBitmap);

    int availableBlocks = 0;
    for (int i = 0; i < super.num_data; i++) {
        if (!(dataBitmap[i / 8] & (1 << (i % 8)))) {
            availableBlocks++;
        }
    }
    int requiredBlocks = (size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;
    int blocksAlreadyUsed = (inode.size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;


    if (availableBlocks < requiredBlocks + blocksAlreadyUsed) {
        return -ENOSPC;
        }
  

    for (int i = 0; i < DIRECT_PTRS; i++) {
        if (inode.direct[i] != 0) {
            char block[UFS_BLOCK_SIZE] = {0};
            disk->writeBlock(inode.direct[i], block);
        }
    }
    int bytesWritten = 0;
    for (int index = 0; bytesWritten < size && bytesWritten < UFS_BLOCK_SIZE; index++) {
        if (index >= DIRECT_PTRS) {
            break;
        }

        int blockNum = inode.direct[index];
        if (blockNum == 0) {
            int dataBitmapBlockNum = -1;
            for (int j = 0; j < super.num_data; j++) {
                if (!(dataBitmap[j / 8] & (1 << (j % 8)))) {
                    dataBitmap[j / 8] |= (1 << (j % 8));
                    dataBitmapBlockNum = j;
                    break;
                }
            }
            if (dataBitmapBlockNum == -1) {
                return -ENOSPC;
            }
            writeDataBitmap(&super, dataBitmap);
            inode.direct[index] = super.data_region_addr + dataBitmapBlockNum;
        }

        char block[UFS_BLOCK_SIZE];
        disk->readBlock(inode.direct[index], block);
        int bytesToWrite = std::min(UFS_BLOCK_SIZE - (bytesWritten % UFS_BLOCK_SIZE), size - bytesWritten);
        memcpy(block + (bytesWritten % UFS_BLOCK_SIZE), static_cast<const char*>(buffer) + bytesWritten, bytesToWrite);
        disk->writeBlock(inode.direct[index], block);
        bytesWritten += bytesToWrite;
    }

    inode.size = bytesWritten;
    
    int inodeBlockNum = super.inode_region_addr + (inodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE;
    int inodeOffset = (inodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;
    char inodeBlock[UFS_BLOCK_SIZE];
    disk->readBlock(inodeBlockNum, inodeBlock);
    memcpy(inodeBlock + inodeOffset, &inode, sizeof(inode_t));
    disk->writeBlock(inodeBlockNum, inodeBlock);

    return bytesWritten;
}*/

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
    if (size <= 0) {
        return -EINVAL;
    }

    inode_t inode;
    super_t super;
    if (stat(inodeNumber, &inode) != 0) {
        return -EINVALIDINODE;
    }

    if (buffer == nullptr) {
        return -EFAULT;
    }

    if (inodeNumber < 0 || inodeNumber == super.num_inodes) {
        return -EINVALIDINODE;
    }

    if (inode.type != UFS_REGULAR_FILE) {
        return -EINVALIDTYPE;
    }
    if (size > MAX_FILE_SIZE) {
        return -EINVALIDSIZE;
    }

    readSuperBlock(&super);
    unsigned char dataBitmap[UFS_BLOCK_SIZE];
    readDataBitmap(&super, dataBitmap);

    int requiredBlocks = (size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;

    vector<int> freeBlocks;
    for (int i = 0; i < super.num_data; i++) {
        if ((dataBitmap[i / 8] & (1 << (i % 8))) == 0) {
            freeBlocks.push_back(super.data_region_addr + i);
        }
        if ((int)freeBlocks.size() == requiredBlocks) {
            break;
        }
    }

    if ((int)freeBlocks.size() < requiredBlocks) {
        return -ENOSPC;
    }

    int bytesWritten = 0;
    for (int i = 0; bytesWritten < size; i++) {
        int blockNum = inode.direct[i];
        if (blockNum == 0) {
            blockNum = freeBlocks[i];
            inode.direct[i] = blockNum;
            dataBitmap[(blockNum - super.data_region_addr) / 8] |= 1 << ((blockNum - super.data_region_addr) % 8);
        }

        char block[UFS_BLOCK_SIZE];
        int bytesToWrite = std::min(size - bytesWritten, UFS_BLOCK_SIZE);
        memcpy(block, static_cast<const char*>(buffer) + bytesWritten, bytesToWrite);
        disk->writeBlock(blockNum, block);
        bytesWritten += bytesToWrite;
    }

    inode.size = size;
    int inodeBlockNum = super.inode_region_addr + (inodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE;
    int inodeOffset = (inodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;
    char inodeBlock[UFS_BLOCK_SIZE];
    disk->readBlock(inodeBlockNum, inodeBlock);
    memcpy(inodeBlock + inodeOffset, &inode, sizeof(inode_t));
    disk->writeBlock(inodeBlockNum, inodeBlock);

    return bytesWritten;
}



int LocalFileSystem::unlink(int parentInodeNumber, string name) {
inode_t parentInode;
super_t super;
readSuperBlock(&super);
    //tests 56,57,58
  if (stat(parentInodeNumber, &parentInode) != 0) {
    return -EINVALIDINODE;
  }

   /* if (name.empty() || name == "." || name == "..") {  //maybe case 58?!
    return -EUNLINKNOTALLOWED;
  }*/

  if (name.empty() || name == "." || name == ".." || name.length() >= DIR_ENT_NAME_SIZE) {
        return -EUNLINKNOTALLOWED;
    }
    if (parentInode.type != UFS_DIRECTORY) {
        return -ENOTDIR;
    }

    //maybe 58?
    if (parentInodeNumber < 0 || parentInodeNumber >= super.num_inodes) {
        return -EINVALIDINODE;
    }

    int inodeToUnlink = -1;
    int dirBlockIndex = -1;
    int dirEntryIndex = -1;
    dir_ent_t dirEntries[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];

     for (int i = 0; i < DIRECT_PTRS && inodeToUnlink == -1; i++) {
        if (parentInode.direct[i] != 0) {
            disk->readBlock(parentInode.direct[i], dirEntries);
            for (size_t j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++) {
                if (dirEntries[j].inum != -1 && strncmp(dirEntries[j].name, name.c_str(), DIR_ENT_NAME_SIZE) == 0) {
                    inodeToUnlink = dirEntries[j].inum;
                    dirBlockIndex = i;
                    dirEntryIndex = j;
                    break;
                }
            }
        }
    }


    if (inodeToUnlink == -1) {    //HOPEFULLYY THIS PASSES 58 PRAYIN, NVM DOES NOT WORK UGH
    return 0;
}

    inode_t inode;
    if (stat(inodeToUnlink, &inode) != 0) {
        return -EINVALIDINODE;
    }



    if (inode.type == UFS_DIRECTORY) {    //rest cases
        if (static_cast<unsigned>(inode.size) > 2 * sizeof(dir_ent_t)) {
            return -EDIRNOTEMPTY;
        }

       
        if (inode.direct[0] != 0) {
            unsigned char dataBitmap[super.data_bitmap_len * UFS_BLOCK_SIZE];
            readDataBitmap(&super, dataBitmap);
            int dataBlockNum = inode.direct[0] - super.data_region_addr;
            dataBitmap[dataBlockNum / 8] &= ~(1 << (dataBlockNum % 8));
            writeDataBitmap(&super, dataBitmap);
        }
    }

    unsigned char dataBitmap[super.data_bitmap_len * UFS_BLOCK_SIZE];
    readDataBitmap(&super, dataBitmap);
    for (int i = 0; i < DIRECT_PTRS; i++) {
        if (inode.direct[i] != 0) {
            int dataBlockNum = inode.direct[i] - super.data_region_addr;
            dataBitmap[dataBlockNum / 8] &= ~(1 << (dataBlockNum % 8));
            cout << "Freed data block " << inode.direct[i] << " (block number " << dataBlockNum << ")" << endl;
        }
    }
    writeDataBitmap(&super, dataBitmap);

    unsigned char inodeBitmap[super.inode_bitmap_len * UFS_BLOCK_SIZE];
    readInodeBitmap(&super, inodeBitmap);
    inodeBitmap[inodeToUnlink / 8] &= ~(1 << (inodeToUnlink % 8));
    writeInodeBitmap(&super, inodeBitmap);
   

    disk->readBlock(parentInode.direct[dirBlockIndex], dirEntries);
    dirEntries[dirEntryIndex].inum = -1;
    memset(dirEntries[dirEntryIndex].name, 0, DIR_ENT_NAME_SIZE);
    disk->writeBlock(parentInode.direct[dirBlockIndex], dirEntries);
   

    parentInode.size -= sizeof(dir_ent_t);
    int parentInodeBlockNum = super.inode_region_addr + (parentInodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE;
    int parentInodeOffset = (parentInodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;
    char parentBlock[UFS_BLOCK_SIZE];
    disk->readBlock(parentInodeBlockNum, parentBlock);
    memcpy(parentBlock + parentInodeOffset, &parentInode, sizeof(inode_t));
    disk->writeBlock(parentInodeBlockNum, parentBlock);
   
    return 0;
}


void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
    // Read the inode bitmap from the disk
    int startBlock = super->inode_bitmap_addr;
    int numBlocks = super->inode_bitmap_len / UFS_BLOCK_SIZE + 1;
    for (int i = 0; i < numBlocks; i++) {
        disk->readBlock(startBlock + i, inodeBitmap + i * UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
    // Write the inode bitmap to the disk
    int startBlock = super->inode_bitmap_addr;
    int numBlocks = super->inode_bitmap_len / UFS_BLOCK_SIZE + 1;
    for (int i = 0; i < numBlocks; i++) {
        disk->writeBlock(startBlock + i, inodeBitmap + i * UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::readDataBitmap(super_t* super, unsigned char* dataBitmap) {
    for (int i = 0; i < super->data_bitmap_len; i++) {
        disk->readBlock(super->data_bitmap_addr + i, dataBitmap + (i * UFS_BLOCK_SIZE));
    }
}

void LocalFileSystem::writeDataBitmap(super_t* super, unsigned char* dataBitmap) {
    for (int i = 0; i < super->data_bitmap_len; i++) {
        disk->writeBlock(super->data_bitmap_addr + i, dataBitmap + (i * UFS_BLOCK_SIZE));
    }
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
    // Read the inode region from the disk
    int startBlock = super->inode_region_addr;
    int numBlocks = super->inode_region_len / UFS_BLOCK_SIZE;
    for (int i = 0; i < numBlocks; i++) {
        disk->readBlock(startBlock + i, (unsigned char *)inodes + i * UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
    // Write the inode region to the disk
    int startBlock = super->inode_region_addr;
    int numBlocks = super->inode_region_len / UFS_BLOCK_SIZE;
    for (int i = 0; i < numBlocks; i++) {
        disk->writeBlock(startBlock + i, (unsigned char *)inodes + i * UFS_BLOCK_SIZE);
    }
}
