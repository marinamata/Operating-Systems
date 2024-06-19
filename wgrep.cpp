#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

using namespace std;

void search_and_print(int fileDescriptor, const char* searchTerm) {
    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    ssize_t bytesRead;
    string lineBuffer; 

   
    while ((bytesRead = read(fileDescriptor, buffer, bufferSize)) > 0) {
        for (ssize_t i = 0; i < bytesRead; ++i) {
            lineBuffer += buffer[i]; 
            if (buffer[i] == '\n') {
                if (lineBuffer.find(searchTerm) != string::npos) {
                    cout << lineBuffer; 
                }
                lineBuffer.clear(); // Clear buffer line
            }
        }
    }

    if (!lineBuffer.empty() && lineBuffer.find(searchTerm) != string::npos) {
        cout << lineBuffer;
    }

    if (bytesRead == -1) {
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    
    const char* searchTerm = argv[1];

    //testcase3
    if (argc < 2) {
        cout << "wgrep: searchterm [file ...]" << endl;
        exit(1);
    }

    if (argc == 2) {
        // No file is specified
        search_and_print(STDIN_FILENO, searchTerm);
    } else {
        for (int i = 2; i < argc; i++) {
            int fileDescriptor = open(argv[i], O_RDONLY);
            if (fileDescriptor == -1) {
                if (errno == ENOENT) {  //erno a variable for errors: https://www.geeksforgeeks.org/how-to-use-errno-in-cpp/
                    cout << "wgrep: cannot open file" << endl; //testcase2
                } 
                exit(1);
            }

            search_and_print(fileDescriptor, searchTerm);
            close(fileDescriptor);
        }
    }

    exit(0); // everything else should exit 0
}