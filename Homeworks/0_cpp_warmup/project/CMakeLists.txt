cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

project(CppPractices VERSION 1.0)
message(STATUS "[Project] ${PROJECT_NAME} ${PROJECT_VERSION}")

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set compiler path
set(CMAKE_CXX_COMPILER "D:/msys64/ucrt64/bin/g++.exe")
set(CMAKE_C_COMPILER "D:/msys64/ucrt64/bin/gcc.exe")

# 添加包含目录
include_directories(
    "D:/msys64/ucrt64/include"
    "D:/msys64/ucrt64/include/c++/14.2.0"
    "D:/msys64/ucrt64/include/c++/14.2.0/x86_64-w64-mingw32"
    "D:/msys64/ucrt64/lib/gcc/x86_64-w64-mingw32/14.2.0/include"
)

include(cmake/helper.cmake)

set(OUTPUT_PROP   
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_SOURCE_DIR}/bin"
)

add_subdirectory(src)