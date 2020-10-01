#include "common.h"
#include "SHMreqchannel.h"
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side, int len) : RequestChannel(_name, _side) {
    s1 = "/SHM_" + my_name + "1";
    s2 = "/SHM_" + my_name + "2";
    len = len;

    shm1 = new SHMQ(s1, len);
    shm2 = new SHMQ(s2, len);

    if(my_side == CLIENT_SIDE) {
        swap(shm1, shm2);
    }
}

SHMRequestChannel::~SHMRequestChannel(){
    delete shm1;
    delete shm2;
}

int SHMRequestChannel::cread(void* msgbuf, int bufcapacity){
    return shm1->my_shm_recv(msgbuf, bufcapacity);
}

int SHMRequestChannel::cwrite(void* msgbuf, int len){
    return shm2->my_shm_send(msgbuf, len);
}

