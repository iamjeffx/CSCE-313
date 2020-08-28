/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;

void get100DataPoints(FIFORequestChannel chan) {
    // request 100 points from each patient
    ofstream outfile("./received/x1.csv");
    timeval start, end;
    gettimeofday(&start, NULL);
    for(int j = 0; j < 100; j++) {
        datamsg d1(1, j * 0.004, 1);
        datamsg d2(1, j * 0.004, 2);

        chan.cwrite(&d1, sizeof(datamsg));
        double r1;
        chan.cread(&r1, sizeof(double));

        chan.cwrite(&d2, sizeof(datamsg));
        double r2;
        chan.cread(&r2, sizeof(double));

        outfile << "Patient 1" << ", time: " << j * 0.004 << "; ecg1: " << r1 << ", ecg2: " << r2 << endl;
    }
    gettimeofday(&end, NULL);
    double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    cout << "Execution time was " << time_taken << " sec" << endl;
    outfile.close();
}

int main(int argc, char *argv[]){
    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

    int patient = -1;
    double time = -1;
    int ecg = -1;
    int cap = -1;
    bool newChan = false;
    stringstream str;

    // Get values from argv
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
        }
        stringstream().swap(str);
    }


    // closing the channel
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));
}
