#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>



using namespace std;


int main(int argc, char* argv[]) {
    //check command line arguments
    if (argc == 1) {
        return 0;
    }
      // Buffer size for reading files
    const size_t bufferSize = 4096;
    char buffer[bufferSize];

/*his example for opening file

int fileDescriptor = open("main.cpp", O_RDONLY);
if (fileDescriptor < 0) {
    cerr << "cannot open file" << endl;
    exit(1);
}*/


    for (int i = 1; i < argc; ++i) {
        int fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor == -1) {
            cout << "wcat: cannot open file" <<endl;
            exit(1);
        }

        // Read and print the contents of the file
        ssize_t bytesRead;
        while ((bytesRead = read(fileDescriptor, buffer, bufferSize)) > 0) {
            if (write(STDOUT_FILENO, buffer, bytesRead) != bytesRead) {
                cerr << "wcat: write error" << endl;
                close(fileDescriptor);
                exit (1);
            }
        }

        if (bytesRead == -1) {
            cerr << "wcat: read error: " << strerror(errno) << endl;
            close(fileDescriptor);
            exit(1);
        }

        close(fileDescriptor);
    }

    exit(0);
}