cmake_minimum_required(VERSION 3.20)
project(mydb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mydb)

target_sources(mydb PRIVATE
    src/main.cpp
    src/page.cpp
    src/page_manager.cpp
    src/schema.cpp
    src/value.cpp
    src/tuple.cpp
    src/heap_page.cpp
    src/wal.cpp
    src/insertion_test.cpp
    src/heap_manager.cpp
)
