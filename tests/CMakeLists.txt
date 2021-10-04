cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(UEFI-bootloader)

set(CMAKE_C_STANDARD 99)

add_executable(format format.c)