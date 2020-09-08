#include <iostream>
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

int main() {
    while(true) {
        cout << "Shelldon Cooper$ ";
        string line;
        getline(cin, line);
        if(line == string("Exit")) {
            cout << "Shell finished" << endl;
            break;
        }
        int pid = fork();
        if(pid == 0) {
            char* args[2] = {const_cast<char *>(line.c_str()), NULL};
            execvp(args[0], args);
        } else {
            waitpid(pid, 0, 0);
        }
    }
    return 0;
}
