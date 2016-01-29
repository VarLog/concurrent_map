cmake_minimum_required(VERSION 3.0)

project("concurrent_map" CXX)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Please select another Build Directory!")
endif()

# Compiler settings
if(MSVC)
    # Force to always compile with W4
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -std=c++11 -fPIC")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -std=c++11")
endif()


file(GLOB HEADERS "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}/*.h")
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}/*.cpp")

# Bin 
set(MAP_BIN "${PROJECT_NAME}")
add_executable(${MAP_BIN} ${SOURCES} ${HEADERS})
set_property(TARGET ${MAP_BIN} PROPERTY CXX_STANDARD 11)

# Tests
if(ENABLE_TESTS STREQUAL "ON")
    enable_testing()

    find_package(GTest REQUIRED)
    if(GTEST_FOUND)
        include_directories("${GTEST_INCLUDE_DIR}")
    else()
        message(SEND_ERROR "Failed to find GTest")
    endif()

    file(GLOB TEST_SOURCES "${PROJECT_SOURCE_DIR}/src/test/*.cpp")

    set(MAP_TEST "${PROJECT_NAME}_test")
    add_executable(${MAP_TEST} ${TEST_SOURCES})
    set_property(TARGET ${MAP_TEST} PROPERTY CXX_STANDARD 11)
    target_link_libraries(${MAP_TEST} ${GTEST_BOTH_LIBRARIES})

    add_test(AllTestsForMap ${MAP_TEST})

endif(ENABLE_TESTS STREQUAL "ON")

