
set (CMAKE_CXX_STANDARD 11)
cmake_minimum_required(VERSION 3.1)
project(gcc_receiver)
INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(../tools)
aux_source_directory(src  DIR_SRCS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -pthread")
add_executable(${PROJECT_NAME} ${DIR_SRCS} ../tools/socket_udp.cpp)
