/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;

int main(int argc, char *argv[]){
    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

    // sending a non-sense message, you need to change this
    // request 1000 points from each patient
    ofstream outfile("./received/x1.csv");
    time_t start, end;
    time(&start);
    for(int i = 1; i <= 15; i++) {
        for(int j = 0; j < 1000; j++) {
            datamsg d1(i, j * 0.004, 1);
            datamsg d2(i, j * 0.004, 2);

            chan.cwrite(&d1, sizeof(datamsg));
            double r1;
            chan.cread(&r1, sizeof(double));

            chan.cwrite(&d2, sizeof(datamsg));
            double r2;
            chan.cread(&r2, sizeof(double));

            outfile << "Patient " << i << ", time: " << j * 0.004 << "; ecg1: " << r1 << ", ecg2: " << r2 << endl;
        }
    }
    time(&end);
    double timeTaken = double(end - start);
    cout << "Execution time was " << timeTaken << " sec" << endl;
    outfile.close();
    // closing the channel
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));
}
