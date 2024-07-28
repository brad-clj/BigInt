add_executable(BigIntStress BigIntStress.cpp)
target_link_libraries(BigIntStress
                      PUBLIC BigInt
                      )
configure_file(BigIntStress.py .)
set(python python3)
if (WIN32)
    set(python py -3)
endif()
add_test(NAME BigIntStress.py
         COMMAND ${python} BigIntStress.py ./BigIntStress 50000 60 100)
