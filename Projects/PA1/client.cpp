/** PA1
 * Jeffrey Xu
 * 09/06/2020
 * jeffreyxu@tamu.edu
 *
 * Client-side of server
 */
#include<sys/wait.h>
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;

void getnDataPoints(FIFORequestChannel &chan, int n, int patient, int ecg, double time) {
    if(time != -1) {
        datamsg msg1(patient, time, 1);
        datamsg msg2(patient, time, 2);
        double r1, r2;
        chan.cwrite(&msg1, sizeof(datamsg));
        chan.cread(&r1, sizeof(double));
        chan.cwrite(&msg2, sizeof(datamsg));
        chan.cread(&r2, sizeof(double));
        if(ecg == -1) {
            cout << time << "," << r1 << "," << r2 << endl;
        } else if(ecg == 1) {
            cout << time << "," << r1 << endl;
        } else if(ecg == 2) {
            cout << time << "," << r2 << endl;
        }
        return;
    }

    // request n points from a patient
    ofstream outfile("./received/x1.csv");
    timeval start, end;
    gettimeofday(&start, NULL);
    for(int j = 0; j < n; j++) {
        datamsg d1(patient, j * 0.004, 1);
        datamsg d2(patient, j * 0.004, 2);
        double r1, r2;

        if(ecg == -1) {
            chan.cwrite(&d1, sizeof(datamsg));
            chan.cread(&r1, sizeof(double));

            chan.cwrite(&d2, sizeof(datamsg));
            chan.cread(&r2, sizeof(double));

            outfile << j * 0.004 << "," << r1 << "," << r2 << endl;
        } else if(ecg == 1){
            chan.cwrite(&d1, sizeof(datamsg));
            chan.cread(&r1, sizeof(double));
            outfile << j * 0.004 << "," << r1 << endl;
        } else if(ecg == 2) {
            chan.cwrite(&d2, sizeof(datamsg));
            chan.cread(&r2, sizeof(double));
            outfile << j * 0.004 << "," << r2 << endl;
        }
    }
    gettimeofday(&end, NULL);
    double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    cout << "Execution time was " << time_taken << " sec" << endl;
    outfile.close();
}

void getDataFromFile(FIFORequestChannel &chan, string file, int capacity) {
    ofstream outfile("./received/" + file, ios::out | ios::binary);

    filemsg f(0, 0);
    char* buf = new char[sizeof(filemsg) + file.size() + 1];
    memcpy(buf, &f, sizeof(filemsg));
    memcpy(buf + sizeof(filemsg), file.c_str(), file.size() + 1);
    chan.cwrite(buf, sizeof(filemsg) + file.size() + 1);

    __int64_t fileLength;
    __int64_t offset = 0;
    int length;
    chan.cread(&fileLength, sizeof(__int64_t));

    cout << "Length of file: " << fileLength << endl;

    char* databuf = new char[capacity];
    while(offset < fileLength) {
        if(fileLength - offset >= capacity) {
            length = capacity;
        } else {
            length = fileLength - offset;
        }
        filemsg d(offset, length);
        memcpy(buf, &d, sizeof(filemsg));
        memcpy(buf + sizeof(filemsg), file.c_str(), file.size() + 1);
        chan.cwrite(buf, sizeof(filemsg) + file.size() + 1);
        chan.cread(databuf, length);
        outfile.write(databuf, length);
        offset += length;
    }

    delete databuf;
    delete buf;
    outfile.close();
}

int main(int argc, char *argv[]){
    // Flag values
    int patient = -1;
    double time = -1;
    int ecg = -1;
    char* cap = "";
    int capacity = MAX_MESSAGE;
    string file = "";
    bool newChan = false;
    MESSAGE_TYPE c = NEWCHANNEL_MSG;
    stringstream str;
    int opt;

    // Get all args
    while ((opt = getopt(argc, argv, "m:p:t:f:e:c")) != -1) {
        switch (opt) {
            case 'm':
                cap = optarg;
                break;
            case 'p':
                patient = atoi(optarg);
                break;
            case 't':
                time = atof(optarg);
                break;
            case 'f':
                file = optarg;
                break;
            case 'e':
                ecg = atoi(optarg);
                break;
            case 'c':
                newChan = true;
                break;
        }
    }

    // Child process to execute server
    int pid = fork();
    if(pid == 0) {
        char* args[4];
        args[0] = "./server";
        if(cap != "") {
            args[1] = "-m";
            args[2] = cap;
            args[3] = NULL;
        } else {
            args[1] = NULL;
        }
        execvp("./server", args);
    } else {
        FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

        // Case handler for each flag
        double r;
        if(cap != "") {
            capacity = atoi(cap);
        }
        if(newChan) {
            chan.cwrite(&c, sizeof(MESSAGE_TYPE));
            char *newc = new char[MAX_MESSAGE];
            chan.cread(newc, MAX_MESSAGE);
            FIFORequestChannel chan2(newc, FIFORequestChannel::CLIENT_SIDE);
            cout << "Channel 2 created" << endl;

            if(patient != -1) {
                getnDataPoints(chan2, 1000, patient, ecg, time);
            }
            if(file != "") {
                getDataFromFile(chan2, file, capacity);
            }

            MESSAGE_TYPE m1 = QUIT_MSG;
            chan2.cwrite(&m1, sizeof(MESSAGE_TYPE));
            cout << "Channel 2 closed" << endl;
        } else {
            if(patient != -1) {
                getnDataPoints(chan, 1000, patient, ecg, time);
            }
            if(file != "") {
                getDataFromFile(chan, file, capacity);
            }
        }

        cout << "Everything finished" << endl;
        // closing the channel
        MESSAGE_TYPE m = QUIT_MSG;
        chan.cwrite(&m, sizeof (MESSAGE_TYPE));
        cout << "Channel closed" << endl;
        wait(0);
    }
}
