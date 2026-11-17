cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(GDDLLadder VERSION 1.0.0)

if (APPLE AND NOT IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

# All sources live directly in src/
file(GLOB SOURCES src/*.cpp src/*.hpp)

include_directories(src)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define the GEODE_SDK environment variable")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
