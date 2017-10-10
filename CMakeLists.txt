cmake_minimum_required(VERSION 3.8)
project(huff)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        huff/huff.cpp
        huff/stdafx.cpp
        huff/stdafx.h
        huff/targetver.h)

add_executable(huff ${SOURCE_FILES})