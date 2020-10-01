all: server client

common.o: common.h common.cpp
	g++ -g -w -std=c++11 -c common.cpp

FIFOreqchannel.o: FIFOreqchannel.h FIFOreqchannel.cpp
	g++ -g -w -std=c++11 -c FIFOreqchannel.cpp FIFOreqchannel.h

MQreqchannel.o: MQreqchannel.cpp MQreqchannel.h
	g++ -g -w -std=c++11 -c MQreqchannel.cpp MQreqchannel.h

SHMreqchannel.o: SHMreqchannel.cpp SHMreqchannel.h
	g++ -g -w -std=c++11 -c SHMreqchannel.cpp SHMreqchannel.h

client: client.cpp FIFOreqchannel.o MQreqchannel.o SHMreqchannel.o common.o
	g++ -g -w -std=c++11 -o client client.cpp FIFOreqchannel.o MQreqchannel.o SHMreqchannel.o common.o -lpthread -lrt

server: server.cpp  FIFOreqchannel.o MQreqchannel.o common.o
	g++ -g -w -std=c++11 -o server server.cpp FIFOreqchannel.o MQreqchannel.o SHMreqchannel.o common.o -lpthread -lrt

clean:
	rm -rf *.o *.csv *.h.gch fifo* server client data*_*