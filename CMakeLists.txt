cmake_minimum_required(VERSION 3.16)
project(SC3020_Project1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(project1
    src/main.cpp
    src/storage/Record.cpp
    src/storage/Block.cpp
    src/storage/Disk.cpp
    src/bplustree/BPlusTree.cpp
    src/bplustree/Node.cpp
    src/task3/Query.cpp
    src/task3/Benchmark.cpp
)

target_include_directories(project1 PRIVATE
    src
)
