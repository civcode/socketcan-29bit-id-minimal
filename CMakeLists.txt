cmake_minimum_required(VERSION 3.0)

project(socketcan)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmax-errors=5")
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -fmax-errors=5")

set(CMAKE_BUILD_TYPE debug)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(socketcan src/main.cpp)

target_link_libraries(socketcan pthread)


