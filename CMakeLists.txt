cmake_minimum_required(VERSION 3.10)

project(stable_vector)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(my_test my_test.cpp stable_vector.hpp)
target_compile_options(my_test PRIVATE -Wall -Wpedantic)
target_compile_options(my_test PRIVATE -g)
#target_compile_options(my_test PRIVATE -fsanitize=address)
#target_link_options(my_test PRIVATE -fsanitize=address)