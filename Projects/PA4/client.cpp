#include <thread>
#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
using namespace std;


void patient_thread_function(int n, int patientNo, BoundedBuffer* request_buffer){
    datamsg d(patientNo, 0.0, 1);
    double response = 0;

    for(int i = 0; i < n; i++) {
        request_buffer->push((char*)&d, sizeof(datamsg));
        d.seconds += 0.004;
    }
}

void file_thread_function(string filename, BoundedBuffer* request_buffer, FIFORequestChannel* chan, int mb) {
    // 1. Create the file
    string recvfname = "recv/" + filename;

    // Preallocate original length
    char buf[1024];
    filemsg f(0, 0);
    memcpy(buf, &f, sizeof(f));
    strcpy(buf + sizeof(f), filename.c_str());
    chan->cwrite(buf, sizeof(f) + filename.size() + 1);
    __int64_t filelength;
    chan->cread(&filelength, sizeof(filelength));

    FILE* fp = fopen(recvfname.c_str(), "w");
    fseek(fp, filelength, SEEK_SET);
    fclose(fp);

    // 2. Generate all file messages
    filemsg* fm = (filemsg*)buf;
    __int64_t remlen = filelength;

    while(remlen > 0) {
        fm->length = min(remlen, (__int64_t)mb);
        request_buffer->push(buf, sizeof(filemsg) + filename.size() + 1);
        fm->offset += fm->length;
        remlen -= fm->length;
    }
}

void worker_thread_function(FIFORequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc, int mb){
    char buf[1024];
    double response = 0;

    char recvbuf[mb];
    while(true) {
        request_buffer->pop(buf, 1024);
        MESSAGE_TYPE* m = (MESSAGE_TYPE*)buf;

        if(*m == DATA_MSG) {
            chan->cwrite(buf, sizeof(datamsg));
            chan->cread(&response, sizeof(double));
            hc->update(((datamsg*)buf)->person, response);
        } else if(*m == QUIT_MSG) {
            chan->cwrite(m, sizeof(MESSAGE_TYPE));
            delete chan;
            break;
        } else if(*m == FILE_MSG) {
            filemsg* f = (filemsg*)buf;
            string filename = (char*)(f + 1);
            int msize = sizeof(filemsg) + filename.size() + 1;
            chan->cwrite(buf, msize);
            chan->cread(recvbuf, mb);

            string recvfname = "recv/" + filename;

            FILE* fp = fopen(recvfname.c_str(), "r+");
            fseek(fp, f->offset, SEEK_SET);
            fwrite(recvbuf, 1, f->length, fp);
            fclose(fp);
        }
    }
}

FIFORequestChannel* create_new_channel(FIFORequestChannel* mainChan) {
    char name[1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    mainChan->cwrite(&m, sizeof(m));
    mainChan->cread(name, 1024);
    FIFORequestChannel* newChan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
    return newChan;
}

int main(int argc, char *argv[])
{
    int n = 1000;    //default number of requests per "patient"
    int p = -1;     // number of patients [1,15]
    int w = 500;    //default number of worker threads
    int b = 50; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    srand(time_t(NULL));
    string filename = "";

    // Grab command line arguments
    int opt;
    while((opt = getopt(argc, argv, "m:n:p:b:w:f:")) != -1) {
        switch(opt) {
            case 'm':
                m = atoi(optarg);
                break;
            case 'n':
                n = atoi(optarg);
                break;
            case 'p':
                p = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
        }
    }

    int pid = fork();
    if (pid == 0){
		// modify this to pass along m
        execl ("server", "server", (char *)NULL);
    }

	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;

	// Initializing histograms and adding them to hc
    for(int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }

    struct timeval start, end;
    gettimeofday (&start, 0);

    FIFORequestChannel* workerChan[w];
    // Create worker channels
    for(int i = 0; i < w; i++) {
        workerChan[i] = create_new_channel(chan);
    }


    /* Start all threads here */
    thread patient[p];
    for(int i = 0; i < p; i++) {
        patient[i] = thread(patient_thread_function, n, i + 1, &request_buffer);
    }

    thread filethread(file_thread_function, filename, &request_buffer, chan, m);

    thread workers[w];
    for(int i = 0; i < w; i++) {
        workers[i] = thread(worker_thread_function, workerChan[i], &request_buffer, &hc, m);
    }


	/* Join all threads here */
    for(int i = 0; i < p; i++) {
        patient[i].join();
    }
    cout << "Patient threads completed" << endl;

    if(filename != string(""))
        filethread.join();

    for(int i = 0; i < w; i++) {
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char*)&q, sizeof(q));
    }

    for(int i = 0; i < w; i++) {
        workers[i].join();
    }
    cout << "Worker threads completed" << endl;

    gettimeofday (&end, 0);
    // print the results
	hc.print ();

    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;

}
