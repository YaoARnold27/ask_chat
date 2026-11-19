cmake_minimum_required(VERSION 3.15)

# 1. 在 project() 之前指定 vcpkg 工具链与全静态 triplet
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")

# 2. 让 MSVC 静态链接 C/C++ 运行库 (/MT)，不依赖目标机器的 VCRUNTIME140.dll
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(ask_chat C)

# 3. 查找静态版 CURL
find_package(CURL CONFIG REQUIRED)

add_executable(ask_chat main.c)

# 4. 静态链接 libcurl 必须定义 CURL_STATICLIB，否则链接器会寻找 __imp_ 前缀符号报错
target_compile_definitions(ask_chat PRIVATE CURL_STATICLIB)

# 5. 链接 CURL::libcurl（vcpkg 会自动附带 ws2_32, crypt32, secur32, advapi32 等系统依赖）
target_link_libraries(ask_chat PRIVATE CURL::libcurl)