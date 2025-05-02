cmake_minimum_required(VERSION 3.5)
project(Server)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra -Wpedantic)

file(GLOB SOURCES "src/*.cpp")

add_library(ServerLib ${SOURCES})
include_directories(include)

add_executable(server main/main.cpp)


target_link_libraries(server ServerLib)