cmake_minimum_required(VERSION 2.8)
project(wrenly)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")

include_directories(src/)
# the find_package command will look for the Find<package>.cmake file
# in the cmake modules directory
find_package(Wren REQUIRED)
include_directories(${WREN_INCLUDE_DIR})

if (WREN_FOUND)
    add_library(wrenly STATIC src/Wrenly.cpp src/detail/Type.cpp)
else()
    message(FATAL_ERROR "Wren required, but not found.")
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /WX /EHsc")
endif ()
