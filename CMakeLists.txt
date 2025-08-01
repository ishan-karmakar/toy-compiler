cmake_minimum_required(VERSION 3.10)

project(Kaleidoscope CXX)
add_executable(compiler src/lexer.cpp)
target_include_directories(compiler PRIVATE include/)
