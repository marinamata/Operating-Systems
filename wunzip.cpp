#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

/*void compressAndWrite(int fileDescriptor, char &lastChar, uint32_t &runLength) {
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
}*/

void unCompressAndWrite(int fileDescriptor) {
    uint32_t length;
    char character;
    const size_t BUFFER_SIZE = 4096;
    char writeBuffer[BUFFER_SIZE];
    size_t BufferIndex = 0;

    while (read(fileDescriptor, &length, sizeof(length)) == sizeof(length)) {
        if (read(fileDescriptor, &character, sizeof(character)) != sizeof(character)) {
            break;
        }

        for (uint32_t i = 0; i < length; ++i) {
            writeBuffer[BufferIndex++] = character;
            if (BUFFER_SIZE == BufferIndex ) {
                write(STDOUT_FILENO, writeBuffer, BUFFER_SIZE);
                BufferIndex = 0;
            }
        }
    }

    if (BufferIndex > 0) {
        write(STDOUT_FILENO, writeBuffer, BufferIndex);
    }
}

/*int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "wzip: file1 [file2 ...]" << std::endl;
        return 1;
    }
    char lastChar = 0;
    uint32_t runLength = 0;
    for (int i = 1; i < argc; ++i) {
        int fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor == -1) {
            return 2;
        }
        compressAndWrite(fileDescriptor, lastChar, runLength);
        close(fileDescriptor);
    }
if (runLength > 0) {
        transformFormat(runLength);
        write(STDOUT_FILENO, &lastChar, sizeof(lastChar));
    }
    return 0;
}*/
int main(int argc, char *argv[]) {
     if (argc < 2) {
        cout << "wunzip: file1 [file2 ...]" << endl;
        exit(1);
     }
    for (int i = 1; i < argc; ++i) {
        int fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor == -1) {
            exit(2);
        }

        unCompressAndWrite(fileDescriptor);
        close(fileDescriptor);
    }
    
    exit(0);
}
