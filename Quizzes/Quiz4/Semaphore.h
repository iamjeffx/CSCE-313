#ifndef QUIZ4_SEMAPHORE_H
#define QUIZ4_SEMAPHORE_H

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

class Semaphore {
private:
    int value;
//    int max;
    mutex m;
    condition_variable cv;
public:
    Semaphore(int val) : value(val) {}
    void P() {
        unique_lock<mutex> l(m);
        cv.wait(l, [this]{return value > 0;});
        value--;
    }
    void V() {
        unique_lock<mutex> l(m);
        value++;
        cv.notify_all();
    }
};

#endif //QUIZ4_SEMAPHORE_H
