cmake_minimum_required(VERSION 3.17)
project(rabbitmq_stream_c_client C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


add_subdirectory(src/rawClient)



if(WIN32)
    include_directories(/mingw/x86_64-w64-mingw32/lib)
    target_link_libraries(rabbitmq_stream_c_client libws2_32.a)
endif()

if(TARGET_GROUP STREQUAL test)
    include(CTest)
    add_subdirectory(external)
    add_subdirectory(tests)
endif()




