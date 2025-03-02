cmake_minimum_required(VERSION 3.30)
project(ATUL)

set(CMAKE_CXX_STANDARD 17)

# ATUL tests
add_library(ATUL STATIC
    include/atul/Function.hpp
)

target_link_libraries(ATUL PUBLIC AUL)

target_include_directories(ATUL PUBLIC ./include)
target_compile_features(ATUL PRIVATE cxx_std_17)

# ATUL tests
option(ATUL_BUILD_TESTS OFF)

if(ATUL_BUILD_TESTS)
    add_subdirectory(external/googletest/)
    add_subdirectory(tests/)
endif()

# Dependencies

add_subdirectory(external/AUL)
