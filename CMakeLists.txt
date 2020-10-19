cmake_minimum_required(VERSION 3.17)
project(osi_lab1 C)



set(CMAKE_C_STANDARD 99)

add_executable(osi_lab1 main.c)

target_link_libraries(osi_lab1 pthread)

