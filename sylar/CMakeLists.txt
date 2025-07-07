cmake_minimum_required(VERSION 3.5)
project(sylar)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O3 -g -Wall -Werror")

# 设置源文件路径
set(LIB_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/sylar/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sylar/util.cpp  
)

# 创建动态库
add_library(sylar SHARED ${LIB_SRC})

# 创建测试可执行文件
add_executable(test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cc)
target_link_libraries(test sylar)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# 包含头文件目录
target_include_directories(sylar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)
target_include_directories(test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)