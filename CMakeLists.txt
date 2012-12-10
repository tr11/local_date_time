CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project(local_date_time)

ADD_EXECUTABLE(unittests tests.cpp)

SET(Boost_USE_STATIC_LIBS   ON)
FIND_PACKAGE(Boost REQUIRED COMPONENTS unit_test_framework date_time system filesystem)

LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
TARGET_LINK_LIBRARIES(unittests ${Boost_LIBRARIES})
SET_PROPERTY(TARGET unittests PROPERTY COMPILE_DEFINITIONS)

SET(CMAKE_CXX_FLAGS "-std=c++0x -Wall --coverage -O0 -g")

MESSAGE(STATUS "  Build Type: " "${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "  C++ flags: " "${CMAKE_CXX_FLAGS}")
