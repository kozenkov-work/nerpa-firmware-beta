#cmake -B build && cmake --build build 
cmake_minimum_required(VERSION 3.15)

project(nerpa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(nerpa
    src/main.cpp
    src/file_loader.cpp
    src/stack.cpp
    src/value.cpp
    src/tokenizer.cpp
    src/vm.cpp
    src/desktop_host.cpp
)

target_include_directories(nerpa PRIVATE src)

if(MSVC)
    target_compile_options(nerpa PRIVATE /W4)
else()
    target_compile_options(nerpa PRIVATE -Wall -Wextra -pedantic)
endif()