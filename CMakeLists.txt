cmake_minimum_required(VERSION 3.10)

project(Kaleidoscope C CXX)

find_package(LLVM REQUIRED CONFIG)

add_executable(compiler src/lexer.cpp src/ast.cpp src/main.cpp)
target_include_directories(compiler PRIVATE include/ ${LLVM_INCLUDE_DIRS})
target_compile_definitions(compiler PRIVATE ${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBRARIES passes)
target_link_libraries(compiler ${LLVM_LIBRARIES})