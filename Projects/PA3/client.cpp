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
#include <sys/resource.h>

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
    } else {
        // Request n points from a patient
        string filename = "./received/x" + to_string(patient) + "_" + chan->name() + ".csv";
        ofstream outfile(filename);
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
}

void getnDataPointsFromVector(vector<RequestChannel*> channels, int n, int patient, int ecg, double time) {
    for(size_t i = 0; i < channels.size(); i++) {
        getnDataPoints(channels.at(i), n, patient, ecg, time);
    }
}

void getDataFromFile(RequestChannel* chan, string file, int capacity) {
    // Create output file
    ofstream outfile("./received/" + file, ios::out | ios::binary);
    timeval start, end;

    // Extract length of file
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

    // Copy over file contents
    char* databuf = new char[capacity];
    gettimeofday(&start, NULL);
    while(offset < fileLength) {
        // Check if remainder is less than capacity
        if(fileLength - offset >= capacity) {
            length = capacity;
        } else {
            length = fileLength - offset;
        }
        // Generate file message and get contents of file
        filemsg d(offset, length);
        memcpy(buf, &d, sizeof(filemsg));
        memcpy(buf + sizeof(filemsg), file.c_str(), file.size() + 1);
        chan->cwrite(buf, sizeof(filemsg) + file.size() + 1);
        chan->cread(databuf, length);

        // Write contents to output file
        outfile.write(databuf, length);

        // Update offset
        offset += length;
    }
    gettimeofday(&end, NULL);

    // Output execution time
    double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    cout << "Execution time was " << time_taken << " sec" << endl;

    // Clean up heap
    delete databuf;
    delete buf;
    outfile.close();
}

void getDataFromFileFromVector(vector<RequestChannel*> channels, string filename, int capacity) {
    // Create output file
    ofstream outfile("./received/" + filename, ios::out | ios::binary);
    timeval start, end;

    // Extract file length
    filemsg f(0, 0);
    char* buf = new char[sizeof(filemsg) + filename.size() + 1];
    memcpy(buf, &f, sizeof(filemsg));
    memcpy(buf + sizeof(filemsg), filename.c_str(), filename.size() + 1);
    channels.at(0)->cwrite(buf, sizeof(filemsg) + filename.size() + 1);

    __int64_t fileLength;
    __int64_t offset = 0;
    int length;
    channels.at(0)->cread(&fileLength, sizeof(__int64_t));
    cout << "Length of file: " << fileLength << endl;

    // Extract number of bytes to copy for each channel
    int quotient = fileLength / channels.size();
    int remainder = fileLength % quotient;

    // Extract data
    for(size_t i = 0; i < channels.size(); i++) {
        // Last iteration may have extra bytes to copy
        if(i == channels.size() - 1) {
            cout << "Bytes to write: " << (quotient + remainder) << endl;
            while(offset < fileLength) {
                if(fileLength - offset >= capacity) {
                    length = capacity;
                } else {
                    length = fileLength - offset;
                }

                // Buffer to hold copied data
                char* databuf = new char[capacity];

                // Generate file message
                filemsg d(offset, length);
                memcpy(buf, &d, sizeof(filemsg));
                memcpy(buf + sizeof(filemsg), filename.c_str(), filename.size() + 1);
                channels.at(i)->cwrite(buf, sizeof(filemsg) + filename.size() + 1);
                channels.at(i)->cread(databuf, length);

                // Write to file
                outfile.write(databuf, length);

                // Update offset
                offset += length;

                // Clean up heap
                delete databuf;
            }
        } else {
            cout << "Bytes to write: " << quotient << endl;
            while(offset < quotient * (i + 1)) {
                // Check if remainder is less than capacity
                if(quotient * (i + 1) - offset >= capacity) {
                    length = capacity;
                } else {
                    length = quotient * (i + 1) - offset;
                }

                // Buffer to hold copied data
                char* databuf = new char[capacity];

                // Generate file message
                filemsg d(offset, length);
                memcpy(buf, &d, sizeof(filemsg));
                memcpy(buf + sizeof(filemsg), filename.c_str(), filename.size() + 1);
                channels.at(i)->cwrite(buf, sizeof(filemsg) + filename.size() + 1);
                channels.at(i)->cread(databuf, length);

                // Write to file
                outfile.write(databuf, length);

                // Update offset
                offset += length;

                // Clean up heap
                delete databuf;
            }
        }
    }

    // Clean up heap
    delete buf;
    outfile.close();
}

void generateChannels(RequestChannel* control_channel, vector<RequestChannel*>& channels, int num_channels, string ipcMode, int capacity) {
    for (int i = 0; i < num_channels; i++) {
        // Get name of new channel
        MESSAGE_TYPE new_channel = NEWCHANNEL_MSG;
        control_channel->cwrite(&new_channel, sizeof(MESSAGE_TYPE));
        char *name = new char[MAX_MESSAGE];
        control_channel->cread(name, MAX_MESSAGE);

        // Generate new channel
        if (ipcMode == string("f")) {
            channels.push_back(new FIFORequestChannel(name, RequestChannel::CLIENT_SIDE));
        } else if (ipcMode == string("q")) {
            channels.push_back(new MQRequestChannel(name, RequestChannel::CLIENT_SIDE, capacity));
        } else if (ipcMode == string("m")) {
            channels.push_back(new SHMRequestChannel(name, RequestChannel::CLIENT_SIDE, capacity));
        } else {
            printf("Invalid IPC Method");
        }
        delete name;
    }
}

void deleteChannel(RequestChannel* channel) {
    // Generate quit message and send to server
    MESSAGE_TYPE quit = QUIT_MSG;
    channel->cwrite(&quit, sizeof(MESSAGE_TYPE));

    // Notify user that channel is closed and delete the channel
    cout << "Channel " << channel->name() << " closed" << endl;
    delete channel;
}

void deleteChannels(vector<RequestChannel*> channels) {
    for(size_t i = 0; i < channels.size(); i++) {
        deleteChannel(channels.at(i));
    }
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
    vector<RequestChannel*> channels;
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

        if(cap != "") {
            capacity = atoi(cap);
        }

        // Create new channel
        RequestChannel* chan = NULL;
        if(ipcMode == string("f"))
            chan = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE);
        else if(ipcMode == string("q"))
            chan = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE, capacity);
        else if(ipcMode == string("m"))
            chan = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, capacity);

        // Create new channels
        if(newChan) {
            // Generate new channels
            generateChannels(chan, channels, numChan, ipcMode, capacity);
            cout << "Channels created" << endl;

            // Extract data points
            if(patient != -1) {
                getnDataPointsFromVector(channels, 1000, patient, ecg, time);
            }
            // Copy file
            if(file != "") {
                getDataFromFileFromVector(channels, file, capacity);
            }

            // Close channel
            deleteChannels(channels);

        } else {
            // Extract data points
            if(patient != -1) {
                cout << "Requesting data point(s)" << endl;
                getnDataPoints(chan, 1000, patient, ecg, time);
                cout << "Data point(s) requested" << endl;
            }
            // Copy file
            if(file != "") {
                getDataFromFile(chan, file, capacity);
            }
        }
        cout << "Everything finished" << endl;

        // Closing the channel
        deleteChannel(chan);
        wait(0);
    }
}
