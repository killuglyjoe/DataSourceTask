# Set the minimum cmake version required by the project
cmake_minimum_required(VERSION 3.6.2)

# VERSION variables policy
cmake_policy(SET CMP0048 NEW)

SET(CMAKE_SYSTEM_NAME Windows)

find_program(CMAKE_C_COMPILER NAMES gcc)
find_program(CMAKE_CXX_COMPILER NAMES g++)
find_program(CMAKE_AR NAMES ar)
find_program(CMAKE_RANLIB NAMES ranlib)
find_program(CMAKE_MAKE_PROGRAM NAMES mingw32-make)

# which compilers to use for C++
set(CMAKE_CXX_COMPILER g++.exe)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_VERBOSE_MAKEFILE ON)

project("DataSourec test task" LANGUAGES CXX)

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
add_library(DataSouce ${HEADERS} ${SOURCES})

set_target_properties(DataSouce PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(DataSouce
    PUBLIC
        include
    PRIVATE
        private)

# find_package(OpenGL)

if (OpenGL_FOUND)
    message("Data types conversion could be done using " - OpenGL::GLU)
    target_link_libraries(DataSouce OpenGL::GL OpenGL::GLU)
    target_compile_options(DataSouce PUBLIC -DWITH_OPEN_GL)
endif()

# Приклад роботи з бібліотекою
add_executable(DataSouceExample
    example/main.cpp
    example/DataSourceFileEmulator.cpp)

target_link_libraries(DataSouceExample DataSouce)
