#include <iostream>
#include <string>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[]) {
    string dir = argv[1];

    struct dirent* direntp;
    DIR* dirp = opendir(dir.c_str());

    while((direntp = readdir(dirp)) != NULL) {
        struct stat sb;
        string file = dir + "/" + direntp->d_name;
        lstat(file.c_str(), &sb);
        string name(direntp->d_name);
        if(S_ISREG(sb.st_mode) && name.find("solar") != std::npos) {
            printf("%s\n", direntp->d_name);
        }
    }
    closedir(dirp);
    return 0;
}