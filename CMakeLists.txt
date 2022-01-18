cmake_minimum_required(VERSION 3.20)
project(cs)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(lib)
add_subdirectory(tests)

add_executable(ets_main main.cpp)
target_link_libraries(ets_main ets)