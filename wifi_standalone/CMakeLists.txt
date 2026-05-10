cmake_minimum_required(VERSION 3.10)
project(wrxtx CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(libtins REQUIRED)
add_executable(wrxtx wrxtx.cpp
        Transmitter.h
        Receiver.h
        DataStructures.h
        MonUtils.h
)

#add_executable(test_mon_utils MonUtils.h)

target_link_libraries(wrxtx PRIVATE tins)