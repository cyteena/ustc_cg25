cmake_minimum_required(VERSION 3.14)

project(
  "USTC_CG_2025_Framework2D"
  VERSION 1.0
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)


message(STATUS "Started CMake for ${PROJECT_NAME} v${PROJECT_VERSION}...\n")
#set(BUILD_SHARED_LIBS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
set(FRAMEWORK2D_DIR "${PROJECT_SOURCE_DIR}")
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(THIRD_PARTY_DIR "${PROJECT_SOURCE_DIR}/third_party")
set(BINARY_DIR "${PROJECT_SOURCE_DIR}/bin")
set(LIBRARY_DIR "${PROJECT_SOURCE_DIR}/libs")

add_subdirectory(third_party)
if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()
add_subdirectory(src)
