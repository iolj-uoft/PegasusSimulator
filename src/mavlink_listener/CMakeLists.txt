cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mavlink_listener
    main.cpp
)

target_link_libraries(mavlink_listener PRIVATE MAVSDK::mavsdk)

if (NOT MSVC)
    target_compile_options(mavlink_listener PRIVATE -Werror -Wall -Wextra)
else()
    target_compile_options(mavlink_listener PRIVATE -W2)
endif()