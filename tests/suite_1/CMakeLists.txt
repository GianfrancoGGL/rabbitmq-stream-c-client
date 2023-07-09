include_directories(../../src/rawClient)
include_directories(../../external/unity/src)
include_directories(../../external)

link_directories(../../src/rawClient)
link_directories(../../external)

add_executable(client_test
               rawClientTest.c
               )

target_link_libraries(client_test
                      librabbitmq_stream_c_lib.a
                      libUnity.a
                      )
                      
if(WIN32)
target_link_libraries(client_test
                      libws2_32.a)
endif()

add_test(suite_1_test client_test)