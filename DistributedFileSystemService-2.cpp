#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include "DistributedFileSystemService.h"
#include "ClientError.h"
#include "ufs.h"
#include "WwwFormEncodedDict.h"
#include "Disk.h"
#include <cstring>

using namespace std;

DistributedFileSystemService::DistributedFileSystemService(string diskFile) : HttpService("/ds3/") {
    this->fileSystem = new LocalFileSystem(new Disk(diskFile, UFS_BLOCK_SIZE));
}

void DistributedFileSystemService::get(HTTPRequest *request, HTTPResponse *response) {
    string myPath = request->getPath();
    string result;
    struct stat myPath_stat;

    if (stat(myPath.c_str(), &myPath_stat) == 0) {
        if (S_ISDIR(myPath_stat.st_mode)) {
            DIR *dir;
            struct dirent *entry;
            vector<string> entries;

            if ((dir = opendir(myPath.c_str())) != NULL) {
                while ((entry = readdir(dir)) != NULL) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        string entryName = entry->d_name;
                        if (entry->d_type == DT_DIR) {
                            entryName += "/";
                        }
                        entries.push_back(entryName);
                    }
                }
                closedir(dir);
                sort(entries.begin(), entries.end());

                for (const auto& entry : entries) {
                    result += entry + "\n";
                }
            } else {
                throw ClientError::notFound();
            }
        } else {
            int fd = open(myPath.c_str(), O_RDONLY);
            if (fd < 0) {
                throw ClientError::notFound();
            } else {
                char buffer[4096];
                ssize_t bytesRead;

                while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
                    result.append(buffer, bytesRead);
                }

                close(fd);
            }
        }
    } else {
        throw ClientError::notFound();
    }

    response->setBody(result);
}

void DistributedFileSystemService::put(HTTPRequest *request, HTTPResponse *response) {
    string path = request->getPath();
    string data = request->getBody();

    if (path.find("/ds3/") == 0) {
        path = path.substr(5);
    }

    if (path.back() == '/') {
        throw ClientError::badRequest();
        return;
    }

    string dirPath = path.substr(0, path.rfind('/'));
    struct stat path_stat;
    size_t pos = 0;

    while ((pos = dirPath.find('/', pos)) != string::npos) {
        string currentDir = dirPath.substr(0, pos);
        if (stat(currentDir.c_str(), &path_stat) == 0) {
            if (!S_ISDIR(path_stat.st_mode)) {
                throw ClientError::conflict();
            }
        } else {
            if (mkdir(currentDir.c_str(), 0777) != 0) {
                throw ClientError::insufficientStorage();
            }
        }
        pos++;
    }

    fileSystem->disk->beginTransaction();
    try {
        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            fileSystem->disk->rollback();
            throw ClientError::insufficientStorage();
        }

        if (write(fd, data.c_str(), data.size()) != static_cast<ssize_t>(data.size())) {
            close(fd);
            fileSystem->disk->rollback();
            throw ClientError::insufficientStorage();
        }

        close(fd);
        fileSystem->disk->commit();
        response->setStatus(200);
        response->setBody("File created/updated successfully");
    } catch (...) {
        fileSystem->disk->rollback();
        throw;
    }
}

void DistributedFileSystemService::del(HTTPRequest *request, HTTPResponse *response) {
    string myPath = request->getPath();
    struct stat myPath_stat;

    if (stat(myPath.c_str(), &myPath_stat) == 0) {
        if (S_ISDIR(myPath_stat.st_mode)) {
            DIR *dir = opendir(myPath.c_str());
            if (dir) {
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        closedir(dir);
                        throw ClientError::conflict();
                    }
                }
                closedir(dir);
                if (rmdir(myPath.c_str()) != 0) {
                    throw ClientError::insufficientStorage();
                }
            } else {
                throw ClientError::notFound();
            }
        } else {
            if (unlink(myPath.c_str()) != 0) {
                throw ClientError::insufficientStorage();
            }
        }
    } else {
        throw ClientError::notFound();
    }

    response->setBody("");
}
