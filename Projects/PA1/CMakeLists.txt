
cmake_minimum_required(VERSION 3.17)
project(PA1)

set(CMAKE_CXX_STANDARD 14)

add_executable(PA1 Client.cpp common.cpp common.h FIFOreqchannel.cpp FIFOreqchannel.h server.cpp)