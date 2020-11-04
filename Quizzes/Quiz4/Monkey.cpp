#include <iostream>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>

using namespace std;

#define M 50

class Semaphore {
private:
    int value;
    std::mutex m;
    condition_variable cv;
public:
    Semaphore (int _v):value(_v){}
    void P(int weight){
        unique_lock<mutex> l(m);
        cv.wait(l, [this]{return value >= weight;});
        value -= weight;
    }
    void V(int weight) {
        unique_lock<mutex> l(m);
        value += weight;
        cv.notify_all();
    }
};

Semaphore capacity(M);
Semaphore dirmtx(1);
Semaphore* m[2] = {new Semaphore(1), new Semaphore(1)};
int monkey_count[2] = {0, 0};

void CrossRavine(int monkeyid, int dir) {
    cout << "Monkey " << monkeyid << " is crossing in direction " << dir << endl;
}

void WaitUntilSafe(int dir, int monkeyweight) {
    m[dir]->P(1);
    monkey_count[dir]++;
    if(monkey_count[dir] == 1) {
        dirmtx.P(1);
    }
    m[dir]->V(1);
    capacity.P(monkeyweight);
}

void DoneWithCrossing(int dir, int monkeyweight) {
    m[dir]->P(1);
    monkey_count[dir]--;
    if(monkey_count[dir] == 0) {
        dirmtx.V(1);
    }
    m[dir]->V(1);
    capacity.V(monkeyweight);
}

void monkey(int monkeyid, int monkeyweight, int dir) {
    WaitUntilSafe(dir, monkeyweight);
    CrossRavine(monkeyid, dir);
    DoneWithCrossing(dir, monkeyweight);
}
