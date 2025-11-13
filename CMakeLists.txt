cmake_minimum_required(VERSION 3.10)

project(TilingCompiler_Core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(compiler_exec
    src/main.cpp
    src/IR.hpp
    src/CodeGen.hpp
)

set_target_properties(compiler_exec PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)