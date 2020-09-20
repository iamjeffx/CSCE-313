/**PA2 - Unix Shell
 * @author: Jeffrey Xu
 * email: jeffreyxu@tamu.edu
 * Date: 09/20/2020
 *
 * Basic implementation of Unix Shell
 */

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
#include <ctime>

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

/**trim
 * @params: string
 * @return: string
 *
 * Removes leading and trailing space within a string including new lines and tabs
 */
string trim(string str) {
    string result = "";
    int start, end;

    // Find first leading index that isn't a space
    for(unsigned int i = 0; i < str.length(); i++) {
        if(str.at(i) == ' ' || str.at(i) == '\n' || str.at(i) == '\t') {
            continue;
        } else {
            start = i;
            break;
        }
    }

    // Find first trailing index that isn't a space
    for(unsigned int i = str.length() - 1; i >= 0; i--) {
        if(str.at(i) == ' ' || str.at(i) == '\n' || str.at(i) == '\t') {
            continue;
        } else {
            end = i + 1;
            break;
        }
    }

    // Create substring that omits leadings and trailing spaces
    result = str.substr(start, end);
    return result;
}

/**split
 * @params: string, char
 * @return: vector<string>
 *
 * Takes in a string and a delimiter and returns a vector containing the parsed tokens based on
 * the delimiter
 */
vector<string> split(string line, char delim=' ') {
    // Remove leading and trailing spaces
    line = trim(line);

    vector<string> tokens;
    stringstream ss(line);
    string token;

    // Continuously feed in tokens and push onto vector
    while(getline(ss, token, delim)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

/**vecToChar
 * @params: vector<string>&
 * @return: const char**
 *
 * Converts a vector of strings to an array of c-strings
 */
const char** vecToChar(vector<string>& tokens) {
    // Create char**
    const char **result = new const char*[tokens.size() + 1];

    // Loop through each token and convert to char*
    for(size_t i = 0; i < tokens.size(); i++) {
        result[i] = tokens.at(i).c_str();
    }
    // Set last element to NULL for execvp
    result[tokens.size()] = NULL;
    return result;
}

/**charInString
 * @params: string, string
 * @return: bool
 *
 * Checks if a string is contained in another string
 */
bool charInString(string line, string element) {
    // Number of possible substrings
    int iterations = line.length() - element.length() + 1;

    // Check all contiguous substrings
    for(int i = 0; i < iterations; i++) {
        if(line.substr(i, element.length()) == string(element)) {
            return true;
        }
    }
    return false;
}

/**elementInVector
 * @params: vector<stirng>&, string
 * @return: bool
 *
 * Returns true if string element in contained within any string in the vector
 */
bool elementInVector(vector<string>& tokens, string element) {
    for(size_t i = 0; i < tokens.size(); i++) {
        // Ignore quotes
        if(tokens.at(i)[0] == '\'' || tokens.at(i)[0] == '\"') {
            continue;
        }
        if(tokens.at(i) == string(element) || charInString(tokens.at(i), element)) {
            return true;
        }
    }
    return false;
}

/**removeElementInString
 * @params: string, string
 * @return: string
 *
 * Removes all occurs of element in line
 */
string removeElementInString(string line, string element) {
    string result = "";

    // Checks for valid index and whether the substring matches
    for(size_t i = 0; i < line.length(); i++) {
        if(i > line.length() - element.length()) {
            // No substring left to check
            result += line[i];
        } else if(line.substr(i, element.length()) == string(element)) {
            // Skip substring
            i += element.length() - 1;
        } else {
            // No match
            result += line[i];
        }
    }
    return result;
}

/**splitVectorIO
 * @params: vector<string>&, string
 * @return: vector<vector<string>>
 *
 * Creates a double vector containing parsed tokens based on the delimiter
 */
vector<vector<string>> splitVectorIO(vector<string>& tokens, string delim) {
    // Return double vector and single vector buffer for parsing
    vector<vector<string>> result;
    vector<string> buffer;

    for(size_t i = 0; i < tokens.size(); i++) {
        // Delimiter hit
        if(tokens.at(i) == string(delim)) {
            // Only push non-empty buffer
            if(buffer.size() > 0) {
                result.push_back(buffer);
                buffer.clear();
            }
        } else if(charInString(tokens.at(i), delim) && tokens.at(i)[0] != '\"' && tokens.at(i)[0] != '\'') {
            // Delimiter is in the string and token is not a quoted string
            result.push_back(buffer);
            buffer.clear();
            buffer.push_back(removeElementInString(tokens.at(i), delim));
        }
        else {
            buffer.push_back(tokens.at(i));
        }
    }
    // Push last buffer if non-empty
    if(buffer.size() > 0) {
        result.push_back(buffer);
    }
    return result;
}

/**removeQuotesVector
 * @params: vector<string>&
 * @return: None
 *
 * Removes leadings and trailing quotes within a vector
 */
void removeQuotesVector(vector<string>& args) {
    for(size_t i = 0; i < args.size(); i++) {
        if(args.at(i)[0] == '\"' || args.at(i)[0] == '\'') {
            // Only take substring without leading and trailing quotes
            string temp = args.at(i).substr(1, args.at(i).size() - 2);
            args.at(i) = temp;
        }
    }
}

/**runCommands
 * @params: vector<vector<string>>, string, bool, vector<int>
 * @return: None
 *
 * Given a double vector containing piped processes, a previous path, a flag determining background processes and
 * a vector containing process pids, runs pipes functions with I/O redirect and functionality for handling background
 * processes.
 */
void runCommands(vector<vector<string>> tokens, string path, bool bg, vector<int>& bgs) {
    // Backup fd for stdin and stdout
    int inBackup = dup(0);
    int outBackup = dup(1);

    // Loop through every single pipe token and execute
    for(size_t i = 0; i < tokens.size(); i++) {
        // Change directory
        vector<string> args = tokens.at(i);
        if(args.at(0) == string("cd")) {
            if(args.at(1) == string("-")) {
                chdir(path.c_str());
            } else {
                chdir(args.at(1).c_str());
            }
        }

        // IO Redirect
        int fdRedirectIn;
        int fdRedirectOut;
        if(elementInVector(args, ">") && elementInVector(args, "<")) {
            vector<vector<string>> in = splitVectorIO(args, "<");
            string out = in.at(in.size() - 1).at(in.at(in.size() - 1).size() - 1);
            in.at(1).at(0) = split(in.at(1).at(0), '>').at(0);
            int pid = fork();
            if(!pid) {
                fdRedirectOut = open(out.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
                dup2(fdRedirectOut, 1);
                fdRedirectIn = open(in.at(1).at(0).c_str(), O_RDONLY | S_IRUSR);
                dup2(fdRedirectIn, 0);
                const char** arguments = vecToChar(in.at(0));
                execvp(arguments[0], (char**)arguments);
            } else {
                waitpid(pid, 0, 0);
                close(fdRedirectOut);
                close(fdRedirectIn);
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
                // Middle pipe process -> must redirect output to next pipe element
                dup2(fd[1], 1);
            } if(i == tokens.size() - 1) {
                // Output IO redirect on last pipe element
                if(elementInVector(args, ">")) {
                    // Split arguments at '>' to get filename
                    vector<vector<string>> out = splitVectorIO(args, ">");
                    fdRedirectOut = open(out.at(1).at(0).c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

                    // Redirect output
                    dup2(fdRedirectOut, 1);

                    // Remove unnecessary arguments
                    vector<string> temp;
                    for(size_t j = 0; j < args.size(); j++) {
                        if(args.at(j)[0] != '>') {
                            temp.push_back(args.at(j));
                        } else {
                            break;
                        }
                    }
                    //Update arguments
                    args = temp;
                }
            } if(i == 0) {
                // Input IO redirect on first pipe process
                if(elementInVector(args, "<")) {
                    // Split arguments at '<' to get input filename
                    vector<vector<string>> in = splitVectorIO(args, "<");
                    fdRedirectIn = open(in.at(1).at(0).c_str(), O_RDONLY | S_IRUSR);

                    // Redirect input IO
                    dup2(fdRedirectIn, 0);

                    // Remove unnecessary arguments
                    vector<string> temp;
                    for(size_t j = 0; j < args.size(); j++) {
                        if(args.at(j)[0] != '<') {
                            temp.push_back(args.at(j));
                        } else {
                            break;
                        }
                    }
                    // Update arguments
                    args = temp;
                }
            }
            // Remove quotes from argument
            removeQuotesVector(args);

            // Convert vector to passable argument in execvp and execute
            const char** arguments = vecToChar(args);
            execvp(arguments[0], (char**)arguments);

            // In the case of a failure, push pid onto bgs to prevent zombie processes and exit
            bgs.push_back(getpid());
            exit(1);
        } else {
            // Parent Process
            if(i == tokens.size() - 1) {
                // Background process check
                if(!bg)
                    waitpid(pid, 0, 0);
                else
                    bgs.push_back(pid);
                close(fd[0]);
            }
            // Push middle pipe commands to bgs
            bgs.push_back(pid);

            // Redirect input IO and close previouos output IO
            dup2(fd[0], 0);
            close(fd[1]);

            // Close IO redirect file descriptors
            close(fdRedirectIn);
            close(fdRedirectOut);
        }
    }
    // Redirect fd in pipe back to normal stdin and stdout
    dup2(inBackup, 0);
    dup2(outBackup, 1);
}

/**backgroundProcess
 * @params: vector<int>&
 * @return: None
 *
 * Checks all pids in the vector and calls wait on all of them
 */
void backgroundProcess(vector<int>& bgs) {
    for(size_t i = 0; i < bgs.size(); i++) {
        // Conditional wait on every pid
        if(waitpid(bgs.at(i), 0, WNOHANG) == bgs.at(i)) {
            // Process was finished -> remove process from vector
            bgs.erase(bgs.begin() + i);
            --i;
        }
    }
}

/**splitOnQuotes
 * @params: string
 * @return: vector<string
 *
 * Splits string into tokens while preserving the structure of quoted strings
 */
vector<string> splitOnQuotes(string line) {
    // Return vector
    vector<string> tokens;

    // Flags to determine current state of parsing (both cannot be true at once)
    bool singleQuote = false;
    bool doubleQuote = false;

    // Buffer token
    string token = "";

    for(unsigned int i = 0; i < line.length(); i++) {
        // Double quote was hit
        if(line[i] == '\"') {
            // singleQuote already open
            if(singleQuote) {
                token += line[i];
                continue;
            } else if(!doubleQuote) {
                // Opening double quote
                if(token != string("")) {
                    // Push if token is non-empty and reset token
                    tokens.push_back(token);
                    token = "";
                }
                token += line[i];
            } else {
                // Closing double quote
                token += line[i];

                // Push entire quoted string onto vector and reset buffer
                tokens.push_back(token);
                token = "";
            }
            // Toggle doubleQuote flag
            doubleQuote = !doubleQuote;
        } else if(line[i] == '\'') {
            // Single quote hit
            if(doubleQuote) {
                // Double quote already open
                token += line[i];
                continue;
            } else if(!singleQuote) {
                // Opening single quote
                if(token != string("")) {
                    // Push on current token if token is non-empty and reset buffer
                    tokens.push_back(token);
                    token = "";
                }
                token += line[i];
            } else {
                // Closing single quote
                token += line[i];

                // Push current token onto vector and reset buffer
                tokens.push_back(token);
                token = "";
            }
            singleQuote = !singleQuote;
        } else if((singleQuote && !doubleQuote) || (!singleQuote && doubleQuote)) {
            // Some quoted string is still open
            token += line[i];
        } else {
            // No quotes are open
            if(line[i] != ' ' && line[i] != '|') {
                token += line[i];
            } else {
                // Space or pipe delimiter hit
                if(token != string(""))
                    // Push token onto vector if token is non-empty
                    tokens.push_back(trim(token));
                if(line[i] == '|') {
                    // Push pipe delimiter for later parsing
                    tokens.push_back("|");
                }
                // Reset buffer
                token = "";
            }
        }
    }
    // Push last buffer onto vector if token is non-empty
    if(token != string("")) {
        tokens.push_back(token);
    }
    return tokens;
}

/**parsePipeProcesses
 * @params: vector<string>
 * @return: vector<vector<string>>
 *
 * Parses tokens into separate pipes
 */
vector<vector<string>> parsePipeProcesses(vector<string>& tokens) {
    vector<vector<string>> args;
    vector<string> buffer;
    for(size_t i = 0; i < tokens.size(); i++) {
        if(tokens.at(i) != "|" || tokens.at(i)[0] != '|') {
            buffer.push_back(tokens.at(i));
        } else {
            // Pipe delimiter hit
            args.push_back(buffer);
            buffer.clear();
        }
    }
    // Push last token onto vector if token is non-empty
    if(buffer.size() > 0) {
        args.push_back(buffer);
    }
    return args;
}

int main() {
    // Maximum file path name
    const int PATHMAX = 1000;
    char path[PATHMAX];

    // Get current time
    time_t now = time(0);
    string dt = string(ctime(&now));
    dt.erase(std::remove(dt.begin(), dt.end(), '\n'), dt.end());

    // First shell prompt
    cout << "Shelldon Cooper-" << getpwuid(geteuid())->pw_name << "-" << dt << ":~" << getcwd(path, sizeof(path)) << "$ ";

    // Variables needed for shell processing
    string line;
    vector<int> bgs;
    string previousPath = "";
    string currentPath = string(path);

    // Shell
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

        // Parse command
        vector<string> tokens = splitOnQuotes(line);
        vector<vector<string>> pipes = parsePipeProcesses(tokens);

        // Execute command
        runCommands(pipes, previousPath, bg, bgs);

        // Update current and previous paths
        previousPath = currentPath;
        currentPath = string(getcwd(path, sizeof(path)));

        // Update current time
        now = time(0);
        dt = string(ctime(&now));
        dt.erase(std::remove(dt.begin(), dt.end(), '\n'), dt.end());

        // Shell prompt
        cout << "Shelldon Cooper-" << getpwuid(geteuid())->pw_name << "-" << dt << ":~" << currentPath << "$ ";
    }
    return 0;
}
