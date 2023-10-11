cmake_minimum_required(VERSION 3.10)

# set the project name
project(ClientReflection)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)

# add the executable
add_library(ClientReflection SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/ClientReflection/Cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/ClientReflection/ClientAPI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/ClientReflection/Pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/ClientReflection/dllmain.cpp
)

set_target_properties(ClientReflection PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

target_include_directories(ClientReflection PUBLIC
    ${PROJECT_SOURCE_DIR}
)
