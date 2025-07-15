cmake_minimum_required(VERSION 3.5)
project(sylar)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O3 -g -Wall -Werror")

include_directories(.)
include_directories(/apps/yaml-cpp/include)
link_directories(/apps/yaml-cpp/lib)

find_library(YAMLCPP yaml-cpp)
message("***", ${YAMLCPP})

set(LIB_SRC 
    sylar/log.cpp
    sylar/util.cpp 
    sylar/config.cc 
    sylar/thread.cc
)

add_library(sylar SHARED ${LIB_SRC})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(LIB_LIB
    sylar 
    pthread
    ${YAMLCPP}
    )

add_executable(test tests/test.cc)
add_dependencies(test sylar)
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)