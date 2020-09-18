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
#include <pwd.h>

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

vector<string> split(string line, char delim=' ') {
    line = trim(line);
    vector<string> tokens;
    stringstream ss(line);
    string token;
    while(getline(ss, token, delim)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

const char** vecToChar(vector<string>& tokens) {
    // Create char**
    const char **result = new const char*[tokens.size() + 1];

    // Loop through each token and convert to char*
    for(size_t i = 0; i < tokens.size(); i++) {
        result[i] = tokens.at(i).c_str();
    }
    result[tokens.size()] = NULL;
    return result;
}

void execute(string line) {
    vector<string> tokens = split(line);
    const char** args = vecToChar(tokens);
    execvp(args[0], (char**)args);
}

void runCommands(vector<string> tokens, string path, bool bg, vector<int>& bgs) {
    // Backup fd for stdin and stdout
    int inBackup = dup(0);
    int outBackup = dup(1);

    // Loop through every single pipe token and execute
    for(size_t i = 0; i < tokens.size(); i++) {
        // Change directory
        vector<string> args = split(tokens.at(i));
        if(args.at(0) == string("cd")) {
            if(args.at(1) == string("-")) {
                cout << "GO TO " << path << endl;
                chdir(path.c_str());
            } else {
                chdir(args.at(1).c_str());
            }
            break;
        }

        // IO Redirect
        int fdRedirectIn;
        int fdRedirectOut;
        if(tokens.at(i).find_first_of(">") != (size_t)-1 && tokens.at(i).find_first_of("<") != (size_t)-1) {
            vector<string> in = split(tokens.at(i), '<');
            string out = split(in.at(1), '>').at(1);
            in.at(1) = split(in.at(1), '>').at(0);
            int pid = fork();
            if(!pid) {
                fdRedirectOut = open(out.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
                dup2(fdRedirectOut, 1);
                fdRedirectIn = open(in.at(1).c_str(), O_RDONLY | S_IRUSR);
                dup2(fdRedirectIn, 0);
                execute(in.at(0));
            } else {
                waitpid(pid, 0, 0);
                close(fdRedirectOut);
                close(fdRedirectIn);
                break;
            }
        }
        if(tokens.at(i).find_first_of(">") != (size_t)-1 && tokens.at(i).find_first_of("<") == (size_t)-1) {
            vector<string> out = split(tokens.at(i), '>');
            int pid = fork();
            if(!pid) {
                fdRedirectOut = open(out.at(1).c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
                dup2(fdRedirectOut, 1);
                execute(out.at(0));
            } else {
                waitpid(pid, 0, 0);
                close(fdRedirectOut);
                break;
            }
        }
        if(tokens.at(i).find_first_of(">") == (size_t)-1 && tokens.at(i).find_first_of("<") != (size_t)-1) {
            vector<string> in = split(tokens.at(i), '<');
            int pid = fork();
            if(!pid) {
                fdRedirectIn = open(in.at(1).c_str(), O_RDONLY | S_IRUSR);
                dup2(fdRedirectIn, 0);
                execute(in.at(0));
            } else {
                waitpid(pid, 0, 0);
                close(fdRedirectIn);
                break;
            }
        }

        // Create pipe
        int fd[2];
        pipe(fd);

        // Fork child process for executing pipe commands
        int pid = fork();
        if(pid == 0) {
            // Child Process
            if(i < tokens.size() - 1) {
                dup2(fd[1], 1);
            }
            execute(tokens.at(i));
            bgs.push_back(getpid());
            exit(1);
        } else {
            // Parent Process
            if(i == tokens.size() - 1) {
                if(!bg)
                    waitpid(pid, 0, 0);
                close(fd[0]);
            }
            bgs.push_back(pid);
            dup2(fd[0], 0);
            close(fd[1]);
        }
    }
    // Redirect fd in pipe back to normal stdin and stdout
    dup2(inBackup, 0);
    dup2(outBackup, 1);
}

void backgroundProcess(vector<int>& bgs) {
    for(size_t i = 0; i < bgs.size(); i++) {
        if(waitpid(bgs.at(i), 0, WNOHANG) == bgs.at(i)) {
            bgs.erase(bgs.begin() + i);
            --i;
        }
    }
}

const char*** doubleVectToChar(vector<vector<string>>& args) {
    const char*** result = new const char**[args.size()];
    for(size_t i = 0; i < args.size(); i++) {
        result[i] = new const char*[args.at(i).size() + 1];
        result[args.at(i).size()] = NULL;
    }
    for(size_t i = 0; i < args.size(); i++) {
        for(size_t j = 0; j < args.at(i).size(); j++) {
            result[i][j] = args.at(i).at(j).c_str();
        }
    }
    return result;
}

vector<string> splitOnDoubleQuotes(string line) {
    vector<string> tokens;
    string token = "";
    bool quote = false;
    for(unsigned int i = 0; i < line.length(); i++) {
        if(line[i] == '\"') {
            if(!quote) {
                if(token != string("")) {
                    tokens.push_back(token);
                    token = "";
                }
                token += line[i];
            } else {
                token += line[i];
                tokens.push_back(token);
                token = "";
            }
            quote = !quote;
        } else if(quote) {
            token += line[i];
        } else {
            if(line[i] != ' ') {
                token += line[i];
            } else {
                if(token != string(""))
                    tokens.push_back(token);
                token = "";
            }
        }
    }
    if(token != string("")) {
        tokens.push_back(token);
    }
    return tokens;
}

vector<string> splitOnSingleQuotes(string line) {
    vector<string> tokens;
    string token = "";
    bool quote = false;
    for(unsigned int i = 0; i < line.length(); i++) {
        if(line[i] == '\'') {
            if(!quote) {
                if(token != string("")) {
                    tokens.push_back(token);
                    token = "";
                }
                token += line[i];
            } else {
                token += line[i];
                tokens.push_back(token);
                token = "";
            }
            quote = !quote;
        } else if(quote) {
            token += line[i];
        } else {
            if(line[i] != ' ') {
                token += line[i];
            } else {
                if(token != string(""))
                    tokens.push_back(token);
                token = "";
            }
        }
    }
    if(token != string("")) {
        tokens.push_back(token);
    }
    return tokens;
}

vector<vector<string>> parsePipeProcesses(vector<string>& tokens) {
    vector<vector<string>> args;
    vector<string> buffer;
    for(size_t i = 0; i < tokens.size(); i++) {
        if(tokens.at(i) != "|") {
            buffer.push_back(tokens.at(i));
        } else {
            args.push_back(buffer);
            buffer.clear();
        }
    }
    return args;
}

int main() {
    const int PATHMAX = 1000;
    char path[PATHMAX];
    cout << "Shelldon Cooper:~" << getcwd(path, sizeof(path)) << "$ ";
    string line;
    vector<int> bgs;
    string previousPath = "";
    string currentPath = string(path);

    while(getline(cin, line)) {
        // Empty input
        if(line == string("")) {
            cout << "Shelldon Cooper$ ";
            continue;
        }

        // Check for exit case
        if(line == string("Exit")) {
            exit(0);
        }

        // Background process handling
        backgroundProcess(bgs);
        bool bg = false;
        if(line[line.length() - 1] == '&') {
            bg = true;
            line = trim(line.substr(0, line.length() - 1));
        }

        // Execute commands
        vector<string> tokens = split(line, '|');
        runCommands(tokens, previousPath, bg, bgs);
        previousPath = currentPath;
        currentPath = string(getcwd(path, sizeof(path)));
        cout << "Shelldon Cooper:~" << currentPath << "$ ";
    }
    return 0;
}
