cmake_minimum_required(VERSION 3.6)
project(starter_code)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        echo_client.c
        echo_server.c
        Makefile utility.h utility.c message.h message.c common.h)

add_executable(starter_code ${SOURCE_FILES} utility.c message.h message.c common.h)