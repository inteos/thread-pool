# -*- coding: UTF-8 -*-
#
# Copyright (c) 2020 by Inteos Sp. z o.o.
# All rights reserved. See LICENSE file for details.
#
cmake_minimum_required (VERSION 3.7)
project (thread-pool)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CMakeDependentOption)
include(GNUInstallDirs)
include(CheckSymbolExists)
include(CheckIncludeFiles)

find_package(Threads REQUIRED)
add_compile_options(-pthread)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/catch2)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

check_include_files(signal.h HAVE_SIGNAL_H)
check_include_files("sys/types.h" HAVE_SYSTYPES_H)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(catch2h catch2/catch.hpp)
set(test-catch src/tests-main.cpp ${catch2h})
set(HEADERS include/SafeQueue.h include/ThreadPool.h)
set(SOURCES src/ThreadPool.cpp)
#add_definitions(-DAFFINITY)

add_executable(test_thread_pool src/test_thread_pool.cpp ${HEADERS} ${SOURCES} ${test-catch})
target_link_libraries(test_thread_pool Threads::Threads)

add_executable(main src/main.cpp ${HEADERS} ${SOURCES})
target_link_libraries(main Threads::Threads)