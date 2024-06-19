#include <iostream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <map>
#include <cstdlib>
#include <string.h>
#include <vector>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>


using namespace std;

vector<string> parsing(string line);
void pwd(vector<string> directory);
void pathChange(vector<string> &paths, vector<string> &command);
void parsing(vector<string> &input, vector<string> &paths, bool hasRedirection, int tracking);
void elseInputs(vector<string> &command, vector<string> &paths, bool hasRedirection, int tracking);
void redirection(vector<string> &command, vector<string> &paths);


vector<string> parsing(string line) {
    vector<string> input;
    string temp;
    unsigned int i = 0;

    while (i < line.length()) {
        switch (line[i]) {
            case ' ':
                if (!temp.empty()) {
                    input.push_back(temp);
                    temp.clear();
                }
                break;
            case '|':
            case '&':
            case '>':
                if (!temp.empty()) {
                    input.push_back(temp);
                    temp.clear();
                }
                temp.push_back(line[i]);
                input.push_back(temp);
                temp.clear();
                break;
            default:
                temp.push_back(line[i]);
                if (i == line.length() - 1) {
                    input.push_back(temp);
                    temp.clear();
                }
                break;
        }
        i++;
    }

    return input;
}

void pathChange(vector<string>& paths, vector<string>& command) {  
    if (command.size() < 2) {
        return; 
    }
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        cerr << "An error has occurred\n";
        return;
    }
   string currentDir(cwd);
    for (size_t i = 1; i < command.size(); ++i) {
        string path = command[i];
        while (!path.empty() && path.back() == '/') {
            path.pop_back();
        }
        if (path.empty() || path.front() == '/') {
            paths.push_back(path + '/'); 
        } else {
            paths.push_back(currentDir + '/' + path + '/'); 
    }
}
}

void pwd(vector<string> directory) {   
    if (directory.size() != 2) {
       cerr << "An error has occurred\n";
        return;
    }
    string path = directory[1];
    for (char& c : path) {
        if (c == '\\') {
            c = '/';
        }
    }
    bool isValidPath = true;
    for (char c : path) {
        if (!isprint(c) || ispunct(c)) {
            isValidPath = false;
            break;
        }
    }
    if (isValidPath) {// If the path is valid
        if (chdir(path.c_str()) != 0) {
            cerr << "An error has occurred\n";
        }
    } else {
        cerr << "An error has occurred\n";
    }
}

unsigned int getRedirectIndex(vector<string>& command) {    
    bool founddirect = false;
    unsigned int temp = 0;
    for(unsigned int j = 0; j < command.size(); j++) {
        if(command[j] == ">" && !founddirect) {
            temp = j;
            founddirect = true;
        }
    }
    return temp;
}
void elseInputs(vector<string> &input, vector<string> &paths, bool redirection, int index) { 
    char **myargs = new char*[input.size() + 1];
    for (size_t i = 0; i < input.size(); ++i) {
        myargs[i] = const_cast<char*>(input[i].c_str());
    }
    myargs[input.size()] = nullptr;

    string inputs;
    bool checker = false;

    for (const auto& path : paths) {
        string candidatePath = path + '/' + input[0];
        if (access(candidatePath.c_str(), X_OK) == 0) {
            inputs = std::move(candidatePath);
            checker = true;
            break;
        }
    }

    if (!checker) {
        cerr << "An error has occurred\n";
        delete[] myargs;
        return;
    }

    pid_t pid = fork();

    if (pid == 0) { // Child process
        if (redirection) {
            const char* filename = input[index + 1].c_str();
            int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
           if (fd == -1) {
                cerr << "An error has occurred\n";
                delete[] myargs;
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);

            vector<const char*> myargsRedir;
            for (size_t i = 0; i < static_cast<size_t>(index); ++i) {
                myargsRedir.push_back(input[i].c_str());
            }
            for (size_t i = static_cast<size_t>(index) + 2; i < input.size(); ++i) {
                myargsRedir.push_back(input[i].c_str());
            }
            myargsRedir.push_back(nullptr);
            execvp(inputs.c_str(), const_cast<char* const*>(myargsRedir.data()));
        }
         else {
            execvp(inputs.c_str(), myargs);
        }

        cerr << "An error has occurred\n";
        //cout << "error: " << errno << endl;
        delete[] myargs;
        exit(1);
    } else if (pid < 0) {
        cerr << "An error has occurred\n";
        delete[] myargs;
    } else {
        delete[] myargs;
    }
}

void redirection(vector<string> &command, vector<string> &paths) {
    size_t redirectIndex = 0;
    bool hasRedirection = false;

    for (size_t i = 0; i < command.size(); ++i) {
        if (command[i] == ">") {
            if (hasRedirection) {
                cerr << "An error has occurred\n";
                return;
            }
            hasRedirection = true;
            redirectIndex = i;
        }
    }

    if (hasRedirection) {
        if (redirectIndex == command.size() - 1) {
            cerr << "An error has occurred\n";
        } else {
            parsing(command, paths, true, redirectIndex);
            //cerr << "An error has occurred\n";
        }
    } else {
        parsing(command, paths, false, 0);
        //cerr << "An error has occurred\n";
    }
}


/*void parsing(vector<string> &input, vector<string> &paths, bool hasRedirection, int tracking) { 
    if(input[0] == "exit") {
        if(input.size() != 1) {
             cerr<<"An error has occurred\n";
        }
        exit(0);
    }
    else if(input[0] == "cd") {
        pwd(input);
    }
    else if(input[0] == "path" && input.size() == 1) {
        while(!paths.empty()){
            paths.pop_back();
        }  
    }
    else if(input[0] == "path"){
      pathChange(paths, input);
    }else{
      if(!paths.empty()) {
        elseInputs(input, paths, hasRedirection, tracking);
          }else{
               cerr<<"An error has occurred\n";
          }
    }    
}*/
void parsing(vector<string> &input, vector<string> &paths, bool hasRedirection, int tracking) {
    if (input[0] == "exit") {
        if (input.size() != 1) {
            cerr << "An error has occurred\n";
        }
        exit(0);
    }
    else if (input[0] == "cd") {
        if (input.size() != 2) {
            cerr << "An error has occurred\n";
            return;
        }
        int result = chdir(input[1].c_str());
        if (result != 0) {
            cerr << "An error has occurred\n";
        }
    }
    else if (input[0] == "path" && input.size() == 1) {
        while (!paths.empty()) {
            paths.pop_back();
        }
    }
    else if (input[0] == "path") {
        pathChange(paths, input);
    }
    else {
        if (!paths.empty()) {
            elseInputs(input, paths, hasRedirection, tracking);
        }
        else {
            cerr << "An error has occurred\n";
        }
    }
}

int main(int argc, char *argv[]) {
    string line;
    vector<string> input;
    vector<vector<string>> inputs;
    vector<string> paths = {"/bin/", "/usr/bin"};
    ifstream inputFile;
    streambuf* cinbuf = cin.rdbuf();

    if (argc == 2) {
        inputFile.open(argv[1]);
        if (inputFile.is_open()) {
            cin.rdbuf(inputFile.rdbuf());
        } else {
            cerr << "An error has occurred\n";
            exit(1);
        }
    }

    while (getline(cin, line)) {
        if (line == "exit") {
            break;
        }

        input = parsing(line);
        inputs.clear();
        vector<string> command;

        for (const auto& arg : input) {
            if (arg == "&") {
                if (!command.empty()) {
                    inputs.emplace_back(command);
                    command.clear();
                }
            } else {
                command.push_back(arg);
            }
        }

        if (!command.empty()) {
            inputs.emplace_back(command);
        }

        for (auto& cmd : inputs) {
            redirection(cmd, paths);
        }

        while (waitpid(-1, nullptr, 0) > 0);
        inputs.clear();
    }

    if (argc == 2) {
        cin.rdbuf(cinbuf);
        inputFile.close();
    }

    exit(0);
}