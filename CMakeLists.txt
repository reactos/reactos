cmake_minimum_required(VERSION 2.8)
project(REACTOS)

add_definitions(-D__REACTOS__)

if(NOT CMAKE_CROSSCOMPILING)

add_definitions(-DTARGET_i386)
add_definitions(-fshort-wchar)

include_directories(${REACTOS_SOURCE_DIR}/tools/unicode)
include_directories(./include)
include_directories(./include/reactos)
include_directories(./include/reactos/wine)

add_subdirectory(tools)
add_subdirectory(lib)

export(TARGETS widl wmc FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake NAMESPACE native- )

else()

# Activate support for asm source files
enable_language(ASM)

set(IMPORT_EXECUTABLES "${REACTOS_SOURCE_DIR}/build/ImportExecutables.cmake" CACHE FILEPATH "Host executables")
include(${IMPORT_EXECUTABLES})

# Debug Options
add_definitions(-DDBG=1 -D_SEH_ENABLE_TRACE -DKDBG=1)

# Version Options
add_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502 -D_WIN32_WINDOWS=0x502 -D_SETUPAPI_VER=0x502)

# Arch Options
add_definitions(-D_M_IX86 -D_X86_ -D__i386__)

# Other
add_definitions(-DUSE_COMPILER_EXCEPTIONS)
add_definitions(-D_USE_32BIT_TIME_T)

# Compiler Core
add_definitions(-gstabs+ -pipe -fms-extensions -fno-set-stack-executable)

# Tuning
add_definitions(-march=pentium -mtune=i686)

# Warnings
add_definitions(-Wall -Wno-char-subscripts -Wpointer-arith -Wno-multichar -Wno-error=uninitialized -Wno-unused-value)

# Optimizations
add_definitions(-Os -fno-strict-aliasing -ftracer -momit-leaf-frame-pointer -mpreferred-stack-boundary=2 -fno-optimize-sibling-calls)

include_directories(./include)
include_directories(./include/psdk)
include_directories(./include/dxsdk)
include_directories(${REACTOS_BINARY_DIR}/include/dxsdk)
include_directories(${REACTOS_BINARY_DIR}/include/psdk)
include_directories(${REACTOS_BINARY_DIR}/include/reactos)
include_directories(./include/crt)
include_directories(./include/crt/mingw32)
include_directories(./include/ddk)
include_directories(./include/ndk)
include_directories(./include/reactos)
include_directories(./include/reactos/libs)

add_subdirectory(include/psdk)
add_subdirectory(include/dxsdk)
add_subdirectory(include/reactos/mc)

add_subdirectory(lib)

endif()

