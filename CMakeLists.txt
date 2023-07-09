add_subdirectory(src/rawClient)

if(TARGET_GROUP STREQUAL test)
    include(CTest)
    add_subdirectory(external)
    add_subdirectory(tests)
endif()