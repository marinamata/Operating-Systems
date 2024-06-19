#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>


void transformFormat(uint32_t lentgt) {
    unsigned char bytes[4];
    bytes[0] = (unsigned char)(lentgt & 0xFF);
    bytes[1] = (unsigned char)((lentgt >> 8) & 0xFF);
    bytes[2] = (unsigned char)((lentgt >> 16) & 0xFF);
    bytes[3] = (unsigned char)((lentgt >> 24) & 0xFF);

    write(STDOUT_FILENO, bytes, sizeof(bytes));
}

void compressAndWrite(int fileDescriptor, char &lastChar, uint32_t &runLength) {
    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = read(fileDescriptor, buffer, BUFFER_SIZE)) > 0) {
        for (int i = 0; i < bytesRead; ++i) {
            if (runLength == 0) {
                lastChar = buffer[i];
                runLength = 1;
            } else if (buffer[i] == lastChar) {
                runLength++;
                if (runLength == INT32_MAX) {
                    transformFormat(runLength);
                    write(STDOUT_FILENO, &lastChar, sizeof(lastChar));
                    runLength = 0; 
                }
            } else {
                transformFormat(runLength);
                write(STDOUT_FILENO, &lastChar, sizeof(lastChar));
                lastChar = buffer[i];
                runLength = 1; 
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "wzip: file1 [file2 ...]" << std::endl;
        exit(1);
    }
    char lastChar = 0;
    uint32_t runLength = 0;
    for (int i = 1; i < argc; ++i) {
        int fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor == -1) {
            exit(2);
        }
        compressAndWrite(fileDescriptor, lastChar, runLength);
        close(fileDescriptor);
    }
if (runLength > 0) {
        transformFormat(runLength);
        write(STDOUT_FILENO, &lastChar, sizeof(lastChar));
    }
    exit(0);
}