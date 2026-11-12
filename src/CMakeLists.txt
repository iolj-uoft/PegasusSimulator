cmake_minimum_required(VERSION 3.15)
project(pegasus_modules LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MAVSDK REQUIRED)

add_subdirectory(mavlink_listener)
