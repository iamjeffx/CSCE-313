#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "Semaphore.h"
using namespace std;

Semaphore threadADone (0);
Semaphore threadBDone (1);
Semaphore threadCDone (1);
Semaphore mtx (1); // will use as mutex
int Bdone = 0;

void ThreadA (){
    while (true){
        threadCDone.P();

        mtx.P();
        cout << "Thread A Done" << endl;
        mtx.V();

        threadADone.V();
        threadADone.V();
    }
}

void ThreadB (){
    while (true){
        threadADone.P();

        mtx.P();
        cout << "Thread B Done --------" << endl;
        mtx.V();
        usleep (500000);

        mtx.P();
        Bdone++;
        if (Bdone == 2){
            threadBDone.V();
            Bdone = 0; 
        }
        mtx.V();
    }
}

void ThreadC() {
    while(true) {
        threadBDone.P();

        mtx.P();
        cout << "-------- Thread C Done --------" << endl;
        mtx.V();

        threadCDone.V();
    }
}

int main (){
    vector<thread> A;
    vector<thread> B;
    vector<thread> C;

    for (int i=0; i< 100; i++)
        A.push_back (thread (ThreadA));

    for (int i=0; i< 300; i++)
        B.push_back(thread (ThreadB));

    for (int i=0; i< 100; i++)
        C.push_back (thread (ThreadC));


    for (int i=0; i<A.size (); i++)
        A [i].join();

    for (int i=0; i<B.size (); i++)
        B [i].join();

    for (int i=0; i<C.size (); i++)
        C [i].join();

}

