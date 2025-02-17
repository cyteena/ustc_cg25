cmake_minimum_required(VERSION 3.27 FATAL_ERROR)


set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_MAKE_PROGRAM "D:/msys64/ucrt64/bin/mingw32-make.exe")
# set compiler path
set(CMAKE_CXX_COMPILER "D:/msys64/ucrt64/bin/g++.exe")
set(CMAKE_C_COMPILER "D:/msys64/ucrt64/bin/gcc.exe")

project(CppPractices VERSION 1.0)
message(STATUS "[Project] ${PROJECT_NAME} ${PROJECT_VERSION}")

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include(cmake/helper.cmake)

set(OUTPUT_PROP   
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_SOURCE_DIR}/bin"
)

message(STATUS "[workspaceFolder || CMAKE_SOURCE_DIR] ${workspaceFolder} || ${CMAKE_SOURCE_DIR}")

add_subdirectory(src)