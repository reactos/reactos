cmake_minimum_required(VERSION 2.8)
project(REACTOS)

add_definitions(-D__REACTOS__)

#-fshort-wchar

if(NOT CMAKE_CROSSCOMPILING)

add_definitions(-DTARGET_i386)

include_directories(${REACTOS_SOURCE_DIR}/tools/unicode)
include_directories(./include)
include_directories(./include/reactos)
include_directories(./include/reactos/wine)

add_subdirectory(tools)

export(TARGETS widl FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake NAMESPACE native- )

else()

set(IMPORT_EXECUTABLES "${REACTOS_SOURCE_DIR}/build/ImportExecutables.cmake" CACHE FILEPATH "Host executables")
include(${IMPORT_EXECUTABLES})

add_definitions(-DDBG=1)
add_definitions(-D_SEH_ENABLE_TRACE)
add_definitions(-DKDBG=1)
add_definitions(-DWINVER=0x502)
add_definitions(-D_WIN32_IE=0x600)
add_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINDOWS=0x502)
add_definitions(-D_SETUPAPI_VER=0x502)
add_definitions(-D_M_IX86)
add_definitions(-D_X86_)
add_definitions(-D__i386__)
add_definitions(-DUSE_COMPILER_EXCEPTIONS)
add_definitions(-D_USE_32BIT_TIME_T)

include_directories(./include)
include_directories(./include/psdk)
include_directories(${REACTOS_BINARY_DIR}/include/psdk)
include_directories(./include/crt)
include_directories(./include/crt/mingw32)
include_directories(./include/ddk)
include_directories(./include/ndk)
include_directories(./include/reactos)
include_directories(./include/reactos/libs)

add_subdirectory(include/psdk)

add_subdirectory(lib)

endif()
