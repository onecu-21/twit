cmake_minimum_required(VERSION 3.16)
project(twit)

set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(twitc
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/codegen.cpp
)

llvm_map_components_to_libnames(llvm_libs
    core
    support
    irreader
    native
    target
    mc
    codegen
)

target_link_libraries(twitc ${llvm_libs})