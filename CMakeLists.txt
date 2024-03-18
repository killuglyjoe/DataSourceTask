# Set the minimum cmake version required by the project
cmake_minimum_required(VERSION 3.6.2)

# VERSION variables policy
cmake_policy(SET CMP0048 NEW)

set(CMAKE_VERBOSE_MAKEFILE ON)

project("DataSource test task" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

add_compile_options(-Wall)

set(HEADERS
    include/globals.h
    include/DataSource.h
    include/DataSourceFile.h
    include/DataSourceBuffer.h
    include/DataSourceEmulator.h
    include/DataSourceController.h
    include/DataSourceFrameRecorder.h
    include/DataSourceFrameProcessor.h
)

set(SOURCES
    private/DataSource.cpp
    private/DataSourceFile.cpp
    private/DataSourceEmulator.cpp
    private/DataSourceController.cpp
    private/DataSourceFrameRecorder.cpp
    private/DataSourceFrameProcessor.cpp
)

# Бібліотека для роботи з даними
add_library(DataSource ${HEADERS} ${SOURCES})

set_target_properties(DataSource PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(DataSource
    PUBLIC include
    PRIVATE private)

# Приклад роботи з бібліотекою
add_executable(DataSourceExample
    example/main.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries(DataSourceExample DataSource)
else()
    target_link_libraries(DataSourceExample DataSource -lpthread)
endif()
