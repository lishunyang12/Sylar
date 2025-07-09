cmake_minimum_required(VERSION 3.5)
project(sylar)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O3 -g -Wall -Werror")

set(LIB_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/sylar/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sylar/util.cpp  
    ${CMAKE_CURRENT_SOURCE_DIR}/sylar/config.cc
)
add_library(sylar SHARED ${LIB_SRC})
target_include_directories(sylar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_executable(test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cc)
target_include_directories(test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)
target_link_libraries(test sylar)

add_executable(test_config ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_config.cc)
target_include_directories(test_config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)
target_link_libraries(test_config sylar)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)



