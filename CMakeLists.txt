cmake_minimum_required(VERSION 3.15)

project(ask_chat C)

find_package(CURL CONFIG REQUIRED)

if (MSVC)
    add_compile_options(/utf-8)
endif()
add_executable(ask_chat main.c)

target_link_libraries(ask_chat PRIVATE CURL::libcurl)