cmake_minimum_required(VERSION 3.22)
project(BigInt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
# SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()


include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG e39786088138f2749d64e9e90e0f9902daa77c40 # v1.15.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
enable_testing()

add_subdirectory(BigInt)
add_subdirectory(BigIntTest)
add_subdirectory(BigIntStress)
