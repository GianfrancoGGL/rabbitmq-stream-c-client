cmake_minimum_required(VERSION 3.17)
project(rabbitmq_stream_c_client_main C)

set(CMAKE_C_STANDARD 11)

include_directories(src/rawClient)

add_executable(rabbitmq_stream_c_client_main
        src/rawClient/rmqsBroker.c
        src/rawClient/rmqsBroker.h
        src/rawClient/rmqsClient.c
        src/rawClient/rmqsClient.h
        src/rawClient/rmqsClientConfiguration.c
        src/rawClient/rmqsClientConfiguration.h
        src/rawClient/rmqsError.c
        src/rawClient/rmqsError.h
        src/rawClient/rmqsGlobal.h
        src/rawClient/rmqsLib.c
        src/rawClient/rmqsLib.h
        src/rawClient/rmqsList.c
        src/rawClient/rmqsList.h
        src/rawClient/rmqsLogger.c
        src/rawClient/rmqsLogger.h
        src/rawClient/rmqsBuffer.c
        src/rawClient/rmqsBuffer.h
        src/rawClient/rmqsMemory.c
        src/rawClient/rmqsMemory.h
        src/rawClient/rmqsMessage.c
        src/rawClient/rmqsMessage.h
        src/rawClient/rmqsMutex.c
        src/rawClient/rmqsMutex.h
        src/rawClient/rmqsNetwork.c
        src/rawClient/rmqsNetwork.h
        src/rawClient/rmqsProducer.c
        src/rawClient/rmqsProducer.h
        src/rawClient/rmqsProtocol.c
        src/rawClient/rmqsProtocol.h
        src/rawClient/rmqsThread.c
        src/rawClient/rmqsThread.h
        src/rawClient/rmqsTimer.c
        src/rawClient/rmqsTimer.h
        src/main.c)
