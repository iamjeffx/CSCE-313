cmake_minimum_required(VERSION 3.17)
project(PA3)

set(CMAKE_CXX_STANDARD 14)

add_executable(PA3 client.cpp common.cpp common.h FIFOreqchannel.cpp FIFOreqchannel.h server.cpp RequestChannel.h MQreqchannel.cpp MQreqchannel.h SHMreqchannel.cpp SHMreqchannel.h)