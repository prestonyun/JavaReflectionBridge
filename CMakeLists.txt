cmake_minimum_required(VERSION 3.10)

# set the project name
project(ClientReflection)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)

# add the executable
add_library(ClientReflection SHARED
    Cache.cpp
    ClientAPI.cpp
    Pipeline.cpp
    dllmain.cpp
)

target_include_directories(ClientReflection PUBLIC
    ${PROJECT_SOURCE_DIR}
)
