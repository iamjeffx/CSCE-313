#include <thread>
#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"

#include <sys/wait.h>
#include <sys/epoll.h>
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

void event_polling_thread(int n, int p, int w, int mb, FIFORequestChannel** wchans, BoundedBuffer* request_buffer, HistogramCollection* hc){
    char buf[1024];
    double response = 0;

    char recvbuf[mb];

    struct epoll_event ev;
    struct epoll_event events[w];

    // Create an empty epoll list
    int epollfd = epoll_create1(0);
    if(epollfd == -1) {
        EXITONERROR("epoll_create1");
    }

    unordered_map<int, int> fd_to_index;
    vector<vector<char>> state(w);

    int sent = 0, rec = 0;

    // Priming all of the channels and adding each rfd to the list
    for(int i = 0; i < w; i++) {
        int sz = request_buffer->pop(buf, 1024);
        wchans[i]->cwrite(buf, sz);

        // Record the state[i]
        state[i] = vector<char>(buf, buf+sz);
        sent++;
        int rfd = wchans[i]->getrfd();
        fcntl(rfd, F_SETFL, O_NONBLOCK);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = rfd;
        fd_to_index[rfd] = i;
        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, rfd, &ev) == -1) {
            EXITONERROR("epoll_ctl: listen_sock");
        }
    }

    // sent = w, rec = 0
    bool quit_rec = false;

    while(true) {
        if(quit_rec && rec == sent) {
            break;
        }
        int nfds = epoll_wait(epollfd, events, w, -1);
        if(nfds == -1) {
            EXITONERROR("epoll_wait");
        }
        for(int i = 0; i < nfds; i++) {
            int rfd = events[i].data.fd;
            int index = fd_to_index[rfd];
            int resp_sz = wchans[index]->cread(recvbuf, mb);
            rec++;

            // Process recvbuf
            vector<char> req = state[index];
            char* request = req.data();

            // Processing response
            MESSAGE_TYPE* m = (MESSAGE_TYPE*) request;
            if(*m == DATA_MSG) {
                hc->update(((datamsg*)request)->person, *(double*)recvbuf);
            } else if(*m == FILE_MSG) {
                filemsg* f = (filemsg*)m;
                string filename = (char*)(f + 1);

                string recvfname = "recv/" + filename;

                FILE* fp = fopen(recvfname.c_str(), "r+");
                fseek(fp, f->offset, SEEK_SET);
                fwrite(recvbuf, 1, f->length, fp);
                fclose(fp);
            }

            // Reuse channel
            if(!quit_rec) {
                int req_sz = request_buffer->pop(buf, sizeof(buf));
                if (*(MESSAGE_TYPE *) buf == QUIT_MSG) {
                    quit_rec = true;
                    continue;
                }
                wchans[index]->cwrite(buf, req_sz);
                state[index] = vector<char>(buf, buf + req_sz);
                sent++;
            }
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
    int n = 15;   // default number of requests per "patient"
    int p = 10;      // number of patients [1,15]
    int w = 10;    // default number of worker threads
    int b = 100; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    srand(time_t(NULL));
    string filename = "";
    bool p_request = false;
    bool f_request = false;
    string m_s = "";

    // Grab command line arguments
    int opt;
    while((opt = getopt(argc, argv, "m:n:p:b:w:f:")) != -1) {
        switch(opt) {
            case 'm':
                m_s = optarg;
                m = atoi(optarg);
                break;
            case 'n':
                n = atoi(optarg);
                break;
            case 'p':
                p = atoi(optarg);
                p_request = true;
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                f_request = true;
                break;
        }
    }

    if(m_s == "") {
        m_s = "256";
    }

    int pid = fork();
    if (pid == 0){
		// modify this to pass along m
        execl ("server", "server", "-m", (char *)m_s.c_str(), (char *)NULL);
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

    /* Start all threads here */
    if(p_request) {
        for(int i = 0; i < w; i++) {
            workerChan[i] = create_new_channel(chan);
        }

        thread patient[p];
        for(int i = 0; i < p; i++) {
            patient[i] = thread(patient_thread_function, n, i + 1, &request_buffer);
        }

        thread evp(event_polling_thread, n, p, w, m, (FIFORequestChannel**)workerChan, &request_buffer, &hc);

        for(int i = 0; i < p; i++) {
            patient[i].join();
        }
        cout << "Patient threads completed" << endl;

        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char*)&q, sizeof(q));

        evp.join();
        cout << "Worker threads completed" << endl;
    }
    if(f_request) {
        for(int i = 0; i < w; i++) {
            workerChan[i] = create_new_channel(chan);
        }

        thread filethread(file_thread_function, filename, &request_buffer, chan, m);

        thread evp(event_polling_thread, n, p, w, m, (FIFORequestChannel**)workerChan, &request_buffer, &hc);

        filethread.join();

        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char*)&q, sizeof(q));

        evp.join();
        cout << "Worker threads completed" << endl;
    }

    gettimeofday (&end, 0);
    // print the results
	hc.print();

    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;

    for(int i = 0; i < w; i++) {
        workerChan[i]->cwrite((char *)&q, sizeof(MESSAGE_TYPE));
        delete workerChan[i];
        cout << "Worker channel " << i << " deleted" << endl;
    }
    cout << "Worker channels deleted" << endl;

    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;

    wait(0);
}
