#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <vector>

#include "Semaphore.h"

#define N 5
#define THINKING 2
#define HUNGRY 1
#define EATING 0

int state[N];
int phil[N] = {0, 1, 2, 3, 4};

Semaphore* m = new Semaphore(1);
vector<Semaphore*> S;

void test(int phnum)
{
    if (state[phnum] == HUNGRY && state[(phnum + 4) % N] != EATING && state[(phnum + 1) % N] != EATING) {
        // state that eating
        state[phnum] = EATING;

        sleep(2);

        printf("Philosopher %d takes chopstick %d and %d\n", phnum + 1, ((phnum + 4) % N) + 1, phnum + 1);
        S[phnum]->V();
    }
}

// take up chopsticks
void take_chopstick(int phnum)
{
    m->P();

    // state that hungry
    state[phnum] = HUNGRY;

    // eat if neighbours are not eating
    test(phnum);

    m->V();

    // if unable to eat wait to be signalled
    S[phnum]->P();
    sleep(1);
}

// put down chopsticks
void put_chopstick(int phnum)
{
    m->P();

    // state that thinking
    state[phnum] = THINKING;

    printf("Philosopher %d putting chopstick %d and %d down\n", phnum + 1, ((phnum + 4) % N) + 1, phnum + 1);

    test((phnum + 4) % N);
    test((phnum + 1) % N);

    m->V();
}

int* philospher(int* num)
{
    while (1) {
        int* i = (int *)num;
        sleep(1);
        take_chopstick(*i);
        sleep(0);
        put_chopstick(*i);
    }
}

int main()
{
    int i;
    pthread_t thread_id[N];

    for(int j = 0; j < N; j++) {
        S.push_back(new Semaphore(0));
    }

    for (i = 0; i < N; i++) {
        // create philosopher processes
        pthread_create(&thread_id[i], NULL, (void* (*)(void*))philospher, &phil[i]);
    }

    for (i = 0; i < N; i++)
        pthread_join(thread_id[i], NULL);

    delete m;
    for(int j = 0; j < N; j++) {
        delete S[j];
    }
}
