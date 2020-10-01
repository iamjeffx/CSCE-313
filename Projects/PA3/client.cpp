/** PA1
 * Jeffrey Xu
 * 09/06/2020
 * jeffreyxu@tamu.edu
 *
 * Client-side of server
 */
#include <sys/wait.h>
#include <mqueue.h>
#include "common.h"
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h";

using namespace std;

void getnDataPoints(RequestChannel* chan, int n, int patient, int ecg, double time) {
    // Time is specified
    if(time != -1) {
        datamsg msg1(patient, time, 1);
        datamsg msg2(patient, time, 2);
        double r1, r2;
        chan->cwrite(&msg1, sizeof(datamsg));
        chan->cread(&r1, sizeof(double));
        chan->cwrite(&msg2, sizeof(datamsg));
        chan->cread(&r2, sizeof(double));
        if(ecg == -1) {
            cout << time << "," << r1 << "," << r2 << endl;
        } else if(ecg == 1) {
            cout << time << "," << r1 << endl;
        } else if(ecg == 2) {
            cout << time << "," << r2 << endl;
        }
        return;
    }

    // Request n points from a patient
    ofstream outfile("./received/x1.csv");
    timeval start, end;
    gettimeofday(&start, NULL);
    for(int j = 0; j < n; j++) {
        datamsg d1(patient, j * 0.004, 1);
        datamsg d2(patient, j * 0.004, 2);
        double r1, r2;

        if(ecg == -1) {
            chan->cwrite(&d1, sizeof(datamsg));
            chan->cread(&r1, sizeof(double));

            chan->cwrite(&d2, sizeof(datamsg));
            chan->cread(&r2, sizeof(double));

            outfile << r1 << "," << r2 << endl;
        } else if(ecg == 1){
            chan->cwrite(&d1, sizeof(datamsg));
            chan->cread(&r1, sizeof(double));
            outfile << r1 << endl;
        } else if(ecg == 2) {
            chan->cwrite(&d2, sizeof(datamsg));
            chan->cread(&r2, sizeof(double));
            outfile << r2 << endl;
        }
    }
    gettimeofday(&end, NULL);
    double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    cout << "Execution time was " << time_taken << " sec" << endl;
    outfile.close();
}

void getDataFromFile(RequestChannel* chan, string file, int capacity) {
    ofstream outfile("./received/" + file, ios::out | ios::binary);
    timeval start, end;

    filemsg f(0, 0);
    char* buf = new char[sizeof(filemsg) + file.size() + 1];
    memcpy(buf, &f, sizeof(filemsg));
    memcpy(buf + sizeof(filemsg), file.c_str(), file.size() + 1);
    chan->cwrite(buf, sizeof(filemsg) + file.size() + 1);

    __int64_t fileLength;
    __int64_t offset = 0;
    int length;
    chan->cread(&fileLength, sizeof(__int64_t));

    cout << "Length of file: " << fileLength << endl;

    char* databuf = new char[capacity];
    gettimeofday(&start, NULL);
    while(offset < fileLength) {
        if(fileLength - offset >= capacity) {
            length = capacity;
        } else {
            length = fileLength - offset;
        }
        filemsg d(offset, length);
        memcpy(buf, &d, sizeof(filemsg));
        memcpy(buf + sizeof(filemsg), file.c_str(), file.size() + 1);
        chan->cwrite(buf, sizeof(filemsg) + file.size() + 1);
        chan->cread(databuf, length);
        outfile.write(databuf, length);
        offset += length;
    }
    gettimeofday(&end, NULL);

    double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    cout << "Execution time was " << time_taken << " sec" << endl;

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
    int numChan = 1;
    MESSAGE_TYPE c = NEWCHANNEL_MSG;
    stringstream str;
    string ipcMode = "f";
    int opt;

    // Get all args
    while ((opt = getopt(argc, argv, "m:p:t:f:e:c:i:")) != -1) {
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
                numChan = atoi(optarg);
                break;
            case 'i':
                ipcMode = optarg;
                break;
        }
    }

    // Child process to execute server
    int pid = fork();
    if(pid == 0) {
        cout << "Child process" << endl;
        char* args[6];
        args[0] = "./server";
        if(cap != "") {
            args[1] = "-m";
            args[2] = cap;
            args[3] = "-i";
            args[4] = (char*)ipcMode.c_str();
            args[5] = NULL;
        } else {
            args[1] = "-i";
            args[2] = (char*)ipcMode.c_str();
            args[3] = NULL;
        }
        execvp("./server", args);
    } else {
        cout << "Parent process" << endl;
        RequestChannel* chan = NULL;
        if(ipcMode == string("f"))
            chan = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE);
        else if(ipcMode == string("q"))
            chan = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE);
        else if(ipcMode == string("s"))
            chan = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, capacity);

        // Case handler for each flag
        if(cap != "") {
            capacity = atoi(cap);
        }
        if(newChan) {
            chan->cwrite(&c, sizeof(MESSAGE_TYPE));
            char *newc = new char[MAX_MESSAGE];
            chan->cread(newc, MAX_MESSAGE);
            RequestChannel* chan2 = NULL;
            if(ipcMode == "f")
                chan2 = new FIFORequestChannel(newc, RequestChannel::CLIENT_SIDE);
            else if(ipcMode == "q")
                chan2 = new MQRequestChannel(newc, RequestChannel::CLIENT_SIDE);
            else if(ipcMode == "s")
                chan2 = new SHMRequestChannel(newc, RequestChannel::CLIENT_SIDE, capacity);
            cout << "Channel 2 created" << endl;

            if(patient != -1) {
                getnDataPoints(chan2, 1000, patient, ecg, time);
            }
            if(file != "") {
                getDataFromFile(chan2, file, capacity);
            }

            MESSAGE_TYPE m1 = QUIT_MSG;
            chan2->cwrite(&m1, sizeof(MESSAGE_TYPE));
            cout << "Channel 2 closed" << endl;
            delete chan2;
        } else {
            if(patient != -1) {
                cout << "Requesting data point(s)" << endl;
                getnDataPoints(chan, 1000, patient, ecg, time);
                cout << "Data point(s) requested" << endl;
            }
            if(file != "") {
                getDataFromFile(chan, file, capacity);
            }
        }

        cout << "Everything finished" << endl;
        // Closing the channel
        MESSAGE_TYPE m = QUIT_MSG;
        chan->cwrite(&m, sizeof (MESSAGE_TYPE));
        cout << "Channel closed" << endl;
        delete chan;
        wait(0);
    }
}
