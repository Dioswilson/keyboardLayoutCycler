cmake_minimum_required(VERSION 3.27)
project(keyboardLayoutCycler C)

set(CMAKE_C_STANDARD 23)

add_executable(keyboardLayoutCycler main.c charBuffer.c charBuffer.h keyboardLayoutCycle.h)
