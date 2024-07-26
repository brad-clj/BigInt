add_executable(Test Test.cpp)
target_link_libraries(Test
                      PUBLIC BigInt
                      PUBLIC GTest::gtest_main
                      )
# add_test(NAME Test COMMAND Test)
gtest_discover_tests(Test)
