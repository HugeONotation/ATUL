cmake_minimum_required(VERSION 3.30)

add_executable(ATUL_tests
    ATUL_tests.cpp
)

target_link_libraries(ATUL_tests PUBLIC ATUL gtest)
