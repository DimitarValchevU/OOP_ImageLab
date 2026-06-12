cmake_minimum_required(VERSION 3.20)
project(ImageLab VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(ImageLab src/main.cpp ${SOURCES})
target_link_libraries(ImageLab PRIVATE Threads::Threads)