cmake_minimum_required(VERSION 3.10)

project(TilingCompiler_Core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(compiler_exec
    src/main.cpp
    src/IRBuilder.cpp
    src/TilingPass.cpp
    src/CodeGenerator.cpp
)

target_include_directories(compiler_exec PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(compiler_exec PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)