cmake_minimum_required(VERSION 3.17)
project(PA4)

set(CMAKE_CXX_STANDARD 14)

add_executable(PA4 BoundedBuffer.h client.cpp common.cpp common.h FIFOreqchannel.cpp FIFOreqchannel.h Histogram.cpp Histogram.h HistogramCollection.h server.cpp)