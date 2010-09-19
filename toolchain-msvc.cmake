
if(NOT ARCH)
set(ARCH i386)
endif(NOT ARCH)

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)

set(CMAKE_C_FLAGS_INIT "/DWIN32 /D_WINDOWS /W1 /Zm1000")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
