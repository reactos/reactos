
if(NOT ARCH)
    set(ARCH i386)
endif()

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER cl)

if(ARCH STREQUAL "arm")
    include(CMakeForceCompiler)
    CMAKE_FORCE_CXX_COMPILER(cl MSVC)
else()
set(CMAKE_CXX_COMPILER cl)
endif()

set(CMAKE_MC_COMPILER mc)
set(CMAKE_RC_COMPILER rc)
if(ARCH STREQUAL "amd64")
    set(CMAKE_ASM_COMPILER ml64)
elseif(ARCH STREQUAL "arm")
    set(CMAKE_ASM_COMPILER armasm)
else()
    set(CMAKE_ASM_COMPILER ml)
endif()
set(CMAKE_ASM_COMPILER_ID "VISUAL")

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE INTERNAL "")

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    add_definitions(-D__i386__)
endif()
