cmake_minimum_required(VERSION 2.8)
project(REACTOS)

include_directories(${REACTOS_SOURCE_DIR}/tools/unicode)

include_directories(./include)

include_directories(./include/reactos)
include_directories(./include/reactos/wine)

add_definitions(-DTARGET_i386)
add_definitions(-D__REACTOS__)

#-fshort-wchar

if(NOT CMAKE_CROSSCOMPILING)

add_subdirectory(tools)

else()

include_directories(./include)
include_directories(./include/crt/mingw32)
include_directories(./include/crt)
include_directories(./include/reactos/libs)
include_directories(./include/ndk)
include_directories(./include/psdk)
include_directories(./include/ddk)

add_subdirectory(lib)

endif()
