cmake_minimum_required(VERSION 3.10)
project(Lua C)

# 添加所有Lua源文件，排除含有main的lua.c和luac.c
file(GLOB LUA_SOURCES *.c)
list(REMOVE_ITEM LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lua.c" "${CMAKE_CURRENT_SOURCE_DIR}/luac.c")

add_library(lua STATIC ${LUA_SOURCES})

# 设置C标准（根据Lua版本需求）
set_target_properties(lua PROPERTIES
    C_STANDARD 99
    C_EXTENSIONS OFF
)

# 包含头文件目录
target_include_directories(lua PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# 链接数学库（Unix系统）
if(UNIX AND NOT APPLE)
    target_link_libraries(lua PUBLIC m)
endif()