
cmake_minimum_required(VERSION 3.0.0)
project(secondary VERSION 0.0.1)

include(CTest)
enable_testing()

add_subdirectory(src)

add_executable(testung src/test_tree.c src/tree.c)
#add_library(secondary src)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
