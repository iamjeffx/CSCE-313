/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;

void getnDataPoints(FIFORequestChannel &chan, int n, int patient) {
    // request n points from each patient
    ofstream outfile("./received/x1.csv");
    timeval start, end;
    gettimeofday(&start, NULL);
    for(int j = 0; j < n; j++) {
        datamsg d1(patient, j * 0.004, 1);
        datamsg d2(patient, j * 0.004, 2);

        chan.cwrite(&d1, sizeof(datamsg));
        double r1;
        chan.cread(&r1, sizeof(double));

        chan.cwrite(&d2, sizeof(datamsg));
        double r2;
        chan.cread(&r2, sizeof(double));

        outfile << "Patient " << patient << ", time: " << j * 0.004 << "; ecg1: " << r1 << ", ecg2: " << r2 << endl;
    }
    gettimeofday(&end, NULL);
    double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    cout << "Execution time was " << time_taken << " sec" << endl;
    outfile.close();
}

void getDataFromFile(FIFORequestChannel &chan, string file) {
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

    char* databuf = new char[MAX_MESSAGE];
    while(offset < fileLength) {
        if(fileLength - offset >= MAX_MESSAGE) {
            length = MAX_MESSAGE;
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
    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

    // Flag values
    int patient = -1;
    double time = -1;
    int ecg = -1;
    int cap = -1;
    string file = "";
    bool newChan = false;
    MESSAGE_TYPE c = NEWCHANNEL_MSG;
    stringstream str;

    // Get values/flags from argv
    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "-m") == 0) {
            str << argv[++i]; str >> cap;
            cout << "Buffer capacity changed to " << cap << endl;
        } else if(strcmp(argv[i], "-c") == 0) {
            newChan = true;
            cout << "Create new channel" << endl;
        } else if(strcmp(argv[i], "-p") == 0) {
            str << argv[++i]; str >> patient;
            cout << "Get data from patient " << patient << endl;
        } else if(strcmp(argv[i], "-t") == 0) {
            str << argv[++i]; str >> time;
            cout << "Get data from time " << time << endl;
        } else if(strcmp(argv[i], "-e") == 0) {
            str << argv[++i]; str >> ecg;
            cout << "Get data from ecg " << ecg << endl;
        } else if(strcmp(argv[i], "-f") == 0) {
            file = argv[++i];
            cout << "Get data from file " << file << endl;
        }
        stringstream().swap(str);
    }

    // Case handler for each flag
    double r;
    if(patient != -1 && time != -1 && ecg != -1) {
        datamsg d(patient, time, ecg);
        chan.cwrite(&d, sizeof(datamsg));
        chan.cread(&r, sizeof(double));
        cout << "Patient " << patient << ", Time " << time << ", ecg " << ecg << ": " << r << endl;
    }
    if(patient != -1 && time == -1 && ecg == -1) {
        cout << "Get 1000 points from patient " << patient << endl;
        getnDataPoints(chan, 1000, patient);
    }
    if(file != "") {
        getDataFromFile(chan, file);
    }
//    if(newChan) {
//        chan.cwrite(&c, sizeof(MESSAGE_TYPE));
//        cout << "New channel created" << endl;
//        char* newc = new char[MAX_MESSAGE];
//        chan.cread(newc, sizeof(MAX_MESSAGE));
//        cout << newc << endl;
//    }


    // closing the channel
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));
    cout << "Channel closed" << endl;
}
