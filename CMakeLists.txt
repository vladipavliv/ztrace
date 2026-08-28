cmake_minimum_required(VERSION 3.20)
project(ZeroTrace VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ztrace INTERFACE)

target_include_directories(ztrace INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

option(ZTRACE_BUILD_EXAMPLES "Build examples" ON)

if(ZTRACE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

message(STATUS "")
message(STATUS "ZeroTrace Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${ZTRACE_BUILD_EXAMPLES}")
message(STATUS "")