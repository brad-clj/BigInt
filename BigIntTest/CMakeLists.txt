add_executable(BigIntTest BigIntTest.cpp)
target_link_libraries(BigIntTest
                      PUBLIC BigInt
                      PUBLIC GTest::gtest_main
                      )
gtest_discover_tests(BigIntTest)
