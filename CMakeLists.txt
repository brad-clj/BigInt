cmake_minimum_required(VERSION 3.22)
project(BigInt)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG e39786088138f2749d64e9e90e0f9902daa77c40 # v1.15.0
)
FetchContent_MakeAvailable(googletest)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
enable_testing()
add_subdirectory(BigInt)
add_subdirectory(Test)
