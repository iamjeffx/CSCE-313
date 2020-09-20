#include "common.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"

using namespace std;


int main(int argc, char *argv[]){

    int c;
    int buffercap = MAX_MESSAGE;
    int p = 0, ecg = 1;
    double t = -1.0;
    bool isnewchan = false;
    bool isfiletransfer = false;
    string filename;


    while ((c = getopt (argc, argv, "p:t:e:m:f:c")) != -1){
        switch (c){
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                ecg = atoi (optarg);
                break;
            case 'm':
                buffercap = atoi (optarg);
                break;
            case 'c':
                isnewchan = true;
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
        }
    }

    // fork part
    if (fork()==0){ // child

        char* args [] = {"./server", "-m", (char *) to_string(buffercap).c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec filed");
            exit (0);
        }
    }

    FIFORequestChannel* control_chan = new FIFORequestChannel ("control", FIFORequestChannel::CLIENT_SIDE);
    FIFORequestChannel* chan = control_chan;
    if (isnewchan){
        cout << "Using the new channel everything following" << endl;
        MESSAGE_TYPE m = NEWCHANNEL_MSG;
        control_chan->cwrite (&m, sizeof (m));
        char newchanname [100];
        control_chan->cread (newchanname, sizeof (newchanname));
        chan = new FIFORequestChannel (newchanname, FIFORequestChannel::CLIENT_SIDE);
        cout << "New channel by the name " << newchanname << " is created" << endl;
        cout << "All further communication will happen through it instead of the main channel" << endl;
    }



    if (!isfiletransfer){   // requesting data msgs
        if (t >= 0){    // 1 data point
            datamsg d (p, t, ecg);
            chan->cwrite (&d, sizeof (d));
            double ecgvalue;
            chan->cread (&ecgvalue, sizeof (double));
            cout << "Ecg " << ecg << " value for patient "<< p << " at time " << t << " is: " << ecgvalue << endl;
            return 0;
        }else{          // bulk (i.e., 1K) data requests
            double ts = 0;
            datamsg d (p, ts, ecg);
            double ecgvalue;
            for (int i=0; i<1000; i++){
                chan->cwrite (&d, sizeof (d));
                chan->cread (&ecgvalue, sizeof (double));
                d.seconds += 0.004; //increment the timestamp by 4ms
                cout << ecgvalue << endl;
            }
            return 0;
        }
    }
    else if (isfiletransfer){
        // part 2 requesting a file
        filemsg f (0,0);  // special first message to get file size
        int to_alloc = sizeof (filemsg) + filename.size() + 1; // extra byte for NULL
        char* buf = new char [to_alloc];
        memcpy (buf, &f, sizeof(filemsg));
        strcpy (buf + sizeof (filemsg), filename.c_str());
        chan->cwrite (buf, to_alloc);
        __int64_t filesize;
        chan->cread (&filesize, sizeof (__int64_t));
        cout << "File size: " << filesize << endl;

        //int transfers = ceil (1.0 * filesize / MAX_MESSAGE);
        filemsg* fm = (filemsg*) buf;
        __int64_t rem = filesize;
        string outfilepath = string("received/") + filename;
        FILE* outfile = fopen (outfilepath.c_str(), "wb");
        fm->offset = 0;

        char* recv_buffer = new char [MAX_MESSAGE];
        while (rem>0){
            fm->length = (int) min (rem, (__int64_t) MAX_MESSAGE);
            chan->cwrite (buf, to_alloc);
            chan->cread (recv_buffer, MAX_MESSAGE);
            fwrite (recv_buffer, 1, fm->length, outfile);
            rem -= fm->length;
            fm->offset += fm->length;
            //cout << fm->offset << endl;
        }
        fclose (outfile);
        delete recv_buffer;
        delete buf;
        cout << "File transfer completed" << endl;
    }


    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite (&q, sizeof (MESSAGE_TYPE));

    if (chan != control_chan){ // this means that the user requested a new channel, so the control_channel must be destroyed as well
        control_chan->cwrite (&q, sizeof (MESSAGE_TYPE));
    }
    // wait for the child process running server
    // this will allow the server to properly do clean up
    // if wait is not used, the server may sometimes crash
    wait (0);

}
