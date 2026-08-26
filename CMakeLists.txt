cmake_minimum_required(VERSION 3.20)
project(Logger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(Logger SHARED src/Logger.cpp)
target_include_directories(Logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(Logger_tests tests/test_Logger.cpp)
target_link_libraries(Logger_tests PRIVATE gtest_main Logger)

include(GoogleTest)
gtest_discover_tests(Logger_tests)