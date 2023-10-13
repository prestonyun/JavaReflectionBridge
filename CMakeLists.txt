cmake_minimum_required(VERSION 3.10)

# set the project name
project(ClientReflection)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)

# add the include directory to the list of directories to be searched for header files
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/win32)

# add the executable
add_library(ClientReflection SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/Cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/ClientThread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/ClientAPI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/Pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientReflection/dllmain.cpp
)

set_target_properties(ClientReflection PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

target_link_libraries(ClientReflection "${PROJECT_SOURCE_DIR}/libs/jvm.lib")

target_include_directories(ClientReflection PUBLIC
    ${PROJECT_SOURCE_DIR}
)
