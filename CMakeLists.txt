cmake_minimum_required(VERSION 3.15)
project(HttpClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# 添加 sol2、lua 子模块
add_subdirectory(external/lua/src)

add_library(sol2 INTERFACE)

# 添加 include 路径
include_directories(
    include
    external/cpp-httplib
    external/sol2/single/include
    external/openssl/include
    external/reframework/include
)

# 添加 OpenSSL lib 路径
link_directories(external/openssl/lib)

# 添加库文件
add_library(HttpClient SHARED
    src/HttpClient.cpp
    src/dllmain.cpp
)

# 添加宏定义以启用 SSL 支持和静态链接 OpenSSL
target_compile_definitions(HttpClient PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
    OPENSSL_STATIC
)

# 链接静态 OpenSSL 库
target_link_libraries(HttpClient PRIVATE
    lua
    libssl
    libcrypto
)

# Visual Studio 特殊设置
if (MSVC)
    add_compile_options(/wd4244 /wd4819 /wd4068 /utf-8)
    add_definitions(-D_UNICODE -DUNICODE)
    target_compile_options(HttpClient PRIVATE /MP)
else()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
