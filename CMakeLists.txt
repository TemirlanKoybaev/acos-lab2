cmake_minimum_required(VERSION 3.5)
project(pingpong_chat CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server server.cpp)
add_executable(client client.cpp)

# Варнинги
if(MSVC)
    target_compile_options(server PRIVATE /W3)
    target_compile_options(client PRIVATE /W3)
else()
    target_compile_options(server PRIVATE -Wall -Wextra -pedantic)
endif()