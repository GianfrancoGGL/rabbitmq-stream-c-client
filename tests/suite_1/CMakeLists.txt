add_executable(client_test
        rawClientTest.c
        )

target_link_libraries(client_test
        rabbitmq_stream_c_lib
        Unity
        )

add_test(suite_1_test client_test)