cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

project(example)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(
	example
	example.cpp
	../src/console.cpp
	../src/threaded_process.cpp
)

target_link_libraries(example ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(example ${CURSES_LIBRARIES})
