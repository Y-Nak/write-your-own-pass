cmake_minimum_required(VERSION 3.22)

set(CUSTOM_PASS_LLVM15_DIR "" CACHE PATH "LLVM installation directory")
list(APPEND CMAKE_PREFIX_PATH "${CUSTOM_PASS_LLVM15_DIR}/lib/cmake/llvm/")
find_package(LLVM 15 REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_subdirectory(lib)
add_subdirectory(test)