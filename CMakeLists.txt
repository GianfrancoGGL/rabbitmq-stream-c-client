cmake_minimum_required(VERSION 3.17)
project(rabbitmq_stream_c_client C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(src/rawClient)
include_directories(tests/rawClientTest)
include_directories(external/unity/src)

add_executable(rabbitmq_stream_c_client
               src/rawClient/rmqsBroker.c
               src/rawClient/rmqsBroker.h
               src/rawClient/rmqsBuffer.c
               src/rawClient/rmqsBuffer.h
               src/rawClient/rmqsClient.c
               src/rawClient/rmqsClient.h
               src/rawClient/rmqsClientConfiguration.c
               src/rawClient/rmqsClientConfiguration.h
               src/rawClient/rmqsConsumer.c
               src/rawClient/rmqsConsumer.h
               src/rawClient/rmqsError.c
               src/rawClient/rmqsError.h
               src/rawClient/rmqsGlobal.h
               src/rawClient/rmqsLib.c
               src/rawClient/rmqsLib.h
               src/rawClient/rmqsList.c
               src/rawClient/rmqsList.h
               src/rawClient/rmqsLogger.c
               src/rawClient/rmqsLogger.h
               src/rawClient/rmqsMemory.c
               src/rawClient/rmqsMemory.h
               src/rawClient/rmqsMessage.c
               src/rawClient/rmqsMessage.h
               src/rawClient/rmqsMutex.c
               src/rawClient/rmqsMutex.h
               src/rawClient/rmqsNetwork.c
               src/rawClient/rmqsNetwork.h
               src/rawClient/rmqsPublisher.c
               src/rawClient/rmqsPublisher.h
               src/rawClient/rmqsProtocol.c
               src/rawClient/rmqsProtocol.h
               src/rawClient/rmqsThread.c
               src/rawClient/rmqsThread.h
               src/rawClient/rmqsTimer.c
               src/rawClient/rmqsTimer.h
               tests/rawClientTest/rawClientTest.c
			   external/unity/src/unity.c)

if(WIN32)
    include_directories(/mingw/x86_64-w64-mingw32/lib)
    target_link_libraries(rabbitmq_stream_c_client libws2_32.a)
endif()