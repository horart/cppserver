cmake_minimum_required(VERSION 3.5)
project(Server)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra -Wpedantic)

file(GLOB SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(ServerLib ${SOURCES})
include_directories(include)

add_executable(server src/main.cpp)


target_link_libraries(server ServerLib)