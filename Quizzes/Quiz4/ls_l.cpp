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

using namespace std;

int octal(int n) {
    int remainder;
    long octal = 0, i = 1;
    while(n != 0) {
        remainder = n % 8;
        n = n / 8;
        octal = octal + (remainder * i);
        i = i * 10;
    }
    return octal;
}

string permissions(int p) {
    string perm[] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
    if(p >= 8 || p < 0) {
        return "";
    } else {
        return perm[p];
    }
}

string parse(int perm) {
    int p = octal(perm);
    string result = "";
    for(int i = 0; i < 3; i++) {
        result = permissions(p % 10) + result;
        p /= 10;
    }
    if(p == 40) {
        result = "d" + result;
    } else {
        result = "-" + result;
    }
    return result;
}

int length(int n) {
    int i = 0;
    while(n != 0) {
        i++;
        n /= 10;
    }
    return i;
}

int determineLength(string dir) {
    struct dirent* direntp;
    DIR *dirp = opendir(dir.c_str());
    int max = 0;
    while((direntp = readdir(dirp)) != NULL) {
        if(direntp->d_name[0] == '.') {
            continue;
        }
        struct stat sb;
        lstat(direntp->d_name, &sb);
        int filelength = sb.st_size;
        int len = length(filelength);
        if(len > max) {
            max = len;
        }
    }
    return max;
}

char* formatdate(char* str, time_t val) {
    strftime(str, 36, "%b %e %H:%M", localtime(&val));
    return str;
}

int main(int argc, char* argv[]) {
    if(argc == 1) {
        return 0;
    }
    string dir = argv[1];
    int len = determineLength(dir);

    struct dirent* direntp;
    DIR *dirp = opendir(dir.c_str());
    while((direntp = readdir(dirp)) != NULL) {
        if(direntp->d_name[0] == '.') {
            continue;
        }
        char date[36];
        struct stat sb;
        lstat(direntp->d_name, &sb);
        cout << parse(sb.st_mode) << " " << sb.st_nlink << " " << sb.st_uid << " " << sb.st_gid << " ";
        cout.width(len);
        cout << std::right;
        cout << sb.st_size << " " << formatdate(date, sb.st_mtime) << " " << direntp->d_name << endl;
    }
    closedir(dirp);

    return 0;
}
