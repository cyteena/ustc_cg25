cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

project(C2Cpp VERSION 1.0)
message(STATUS "[Project] ${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 20)

include(cmake/helper.cmake)

# 添加包含目录
include_directories(
    "D:/msys64/ucrt64/include"
    "D:/msys64/ucrt64/include/c++/14.2.0"  # 修改版本号
    "D:/msys64/ucrt64/include/c++/14.2.0/x86_64-w64-mingw32"  # 修改版本号
    "D:/msys64/ucrt64/lib/gcc/x86_64-w64-mingw32/14.2.0/include"  # 修改版本号
)

set(OUTPUT_PROP   
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_SOURCE_DIR}/bin"
)

add_subdirectory(src)

