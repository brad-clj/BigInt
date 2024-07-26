add_executable(Test Test.cpp)
target_link_libraries(Test
                      PUBLIC BigInt
                      PUBLIC GTest::gtest_main
                      )
gtest_discover_tests(Test)
