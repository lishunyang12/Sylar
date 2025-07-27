cmake_minimum_required(VERSION 3.5)
project(sylar)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O3 -g -Wall \
    -Werror -Wno-unused-function")

include_directories(.)
include_directories(/apps/yaml-cpp/include)
link_directories(/apps/yaml-cpp/lib)

find_library(YAMLCPP NAMES yaml-cpp)
find_library(PTHREAD NAMES pthread)
message("*** YAMLCPP: ${YAMLCPP}")

set(LIB_SRC 
    sylar/config.cc 
    sylar/fiber.cc
    sylar/iomanager.cc
    sylar/log.cpp
    sylar/scheduler.cc
    sylar/thread.cc
    sylar/util.cpp 
)

add_library(sylar SHARED ${LIB_SRC})
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(LIB_LIB sylar pthread ${YAMLCPP})

add_executable(test tests/test.cc)
add_dependencies(test sylar)
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util sylar)
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber sylar)
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager sylar)
target_link_libraries(test_iomanager ${LIB_LIB})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)