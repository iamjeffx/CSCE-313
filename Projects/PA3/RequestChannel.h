#ifndef PA3_REQUESTCHANNEL_H
#define PA3_REQUESTCHANNEL_H

#include <iostream>
#include <string>
#include "common.h"

using namespace std;

class RequestChannel {
public:
    enum Side {SERVER_SIDE, CLIENT_SIDE};
    enum Mode {READ_MODE, WRITE_MODE};
protected:
    string my_name;
    Side my_side;
    int wfd;
    int rfd;

    string s1, s2;
    virtual int open_ipc(string _pipe_name, int mode) {return 0;};

public:
    RequestChannel(const string _name, const Side _side) : my_side(_side), my_name(_name) {};
    virtual ~RequestChannel(){};
    virtual int cwrite(void* buf, int buflen) = 0;
    virtual int cread(void* buf, int buflen) = 0;
    string name() {
        return this->my_name;
    };
};

#endif //PA3_REQUESTCHANNEL_H
