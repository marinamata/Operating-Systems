#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <vector>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

// Function to print the contents of a directory
void printContent(LocalFileSystem &fs, int inodeNumber, const string &path) {
    inode_t inode;
    if (fs.stat(inodeNumber, &inode) != 0) {
        cerr << "Error: Unable to read inode " << inodeNumber << endl;
        return;
    }

    if (inode.type != UFS_DIRECTORY) {
        cerr << "Error: Inode " << inodeNumber << " is not a directory" << endl;
        return;
    }

    static bool isFirstDirectory = true;
    if(isFirstDirectory) {
        cout << "Directory " << path << endl;
        isFirstDirectory = false;
    } else {
        cout << endl;
        cout << "Directory " << path << endl;
    }

    vector<dir_ent_t> entries(inode.size / sizeof(dir_ent_t));
    if (fs.read(inodeNumber, entries.data(), inode.size) != inode.size) {
        cerr << "Error: Unable to read directory entries for inode " << inodeNumber << endl;
        return;
    }

    bool dotPresent = false, dotDotPresent = false;
    for (const auto &entry : entries) {
        if (strcmp(entry.name, ".") == 0) {
            dotPresent = true;
        } else if (strcmp(entry.name, "..") == 0) {
            dotDotPresent = true;
        }
    }

    if (inodeNumber != UFS_ROOT_DIRECTORY_INODE_NUMBER && !dotPresent && !dotDotPresent) {
        dir_ent_t dotEntry;
        dotEntry.inum = inodeNumber;
        strcpy(dotEntry.name, ".");
        entries.push_back(dotEntry);

        dir_ent_t dotDotEntry;
        dotDotEntry.inum = UFS_ROOT_DIRECTORY_INODE_NUMBER;
        strcpy(dotDotEntry.name, "..");
        entries.push_back(dotDotEntry);
    }

    sort(entries.begin(), entries.end(), [](const dir_ent_t &a, const dir_ent_t &b) {
        return strcmp(a.name, b.name) < 0;
    });

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto &entry = entries[i];
        cout << entry.inum << "\t" << entry.name;
        if (i < entries.size() - 1) {
            cout << endl;
        }
    }

    for (const auto &entry : entries) {
        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) {
            continue;
        }
        if (entry.inum != -1) {
            inode_t entryInode;
            if (fs.stat(entry.inum, &entryInode) == 0 && entryInode.type == UFS_DIRECTORY) {
                string newPath = path + entry.name + "/";
                cout << endl;
                printContent(fs, entry.inum, newPath);
            }
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <disk_image>" << endl;
        return 1;
    }

    Disk disk(argv[1], UFS_BLOCK_SIZE);
    LocalFileSystem fs(&disk);

    printContent(fs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "/");

    cout<<endl;
    cout<<endl;

    return 0;
}
