cmake_minimum_required(VERSION 3.14)
project(CppND_System_Monitor)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(CppND_System_Monitor
        constants.h
        main.cpp
        Process.h
        ProcessContainer.h
        ProcessParser.h
        SysInfo.h
        util.h)
