#include <iostream>
#include <stdio.h>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include <sys/time.h>
#include <cassert>
#include <assert.h>

#include <cmath>
#include <numeric>
#include <algorithm>

#include <list>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

string trim(string str) {
    string result = "";
    int start, end;
    for(unsigned int i = 0; i < str.length(); i++) {
        if(str.at(i) == ' ' || str.at(i) == '\n' || str.at(i) == '\t') {
            continue;
        } else {
            start = i;
            break;
        }
    }
    for(unsigned int i = str.length() - 1; i >= 0; i--) {
        if(str.at(i) == ' ' || str.at(i) == '\n' || str.at(i) == '\t') {
            continue;
        } else {
            end = i + 1;
            break;
        }
    }
    result = str.substr(start, end);
    return result;
}

vector<string> split(string line, string delim=" ") {
    line = trim(line);
    vector<string> tokens;
    int index;
    while((index = line.find_first_of(delim)) != -1) {
        string token = trim(line.substr(0, index));
        if(token == "") {
            line = line.substr(index + 1, line.length());
            continue;
        } else {
            tokens.push_back(token);
            line = line.substr(index + 1, line.length());
        }
    }
    tokens.push_back(trim(line));
    return tokens;
}

char** vecToChar(vector<string>& tokens) {
    char** result = new char*[tokens.size() + 1];
    for(size_t i = 0; i < tokens.size(); i++) {
        result[i] = new char[tokens.at(i).length()];
        for(unsigned int j = 0; j < tokens.at(i).length(); j++) {
            result[i][j] = tokens.at(i)[j];
        }
    }
    result[tokens.size()] = NULL;
    return result;
}

void execute(string line) {
    vector<string> tokens = split(line);
    char** args = vecToChar(tokens);
    execvp(args[0], args);
}

int main() {
    vector<int> bgs;
    int inBackup = dup(0);
    int outBackup = dup(1);

    // Begin shell
    cout << "Shelldon Cooper$ ";
    string line;
    getline(cin, line);

    while(true) {
        // Wait on background process
        for(size_t i = 0; i < bgs.size(); i++) {
            if(waitpid(bgs.at(i), 0, WNOHANG) == bgs.at(i)) {
                bgs.erase(bgs.begin() + i);
                --i;
            }
        }

        line = trim(line);
//        bool bg = false;

        // Check for background process and exit statement
        if(line == string("Exit")) {
            cout << "Shell finished" << endl;
            return 0;
        }
//        if(line[line.length() - 1] == '&') {
//            bg = true;
//            line = trim(line.substr(0, line.length() - 1));
//        }

        // Handle pipe processes
        vector<string> pipes = split(line, "|");
        int fd[2];
        for (size_t i = 0; i < pipes.size(); i++) {
            pipe(fd);
            int pid = fork();
            if(!pid) {
                if(i < pipes.size() - 1) {
                    dup2(fd[1], 1);
                }
                execute(pipes.at(i));
            } else {
                if(i == pipes.size() - 1) {
                    waitpid(pid, 0, 0);
                }
                close(fd[1]);
                dup2(fd[0], 0);
            }
        }
//        close(fd[0]);
//        close(fd[1]);
        dup2(inBackup, 0);
        dup2(outBackup, 1);

        cout << "Shelldon Cooper$ ";
        getline(cin, line);
    }
    return 0;
}
