add_executable(Test Test.cpp)
target_link_libraries(Test PRIVATE BigInt)
target_include_directories(Test PRIVATE 
                           "${PROJECT_SOURCE_DIR}/BigInt"
                           )
add_test(NAME Test COMMAND Test)
