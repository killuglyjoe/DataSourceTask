# Append current directory to CMAKE_MODULE_PATH for making device specific cmake modules visible
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Target definition
set(CMAKE_SYSTEM_NAME  Windows)

set(CMAKE_SYSTEM_PROCESSOR x64)

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
