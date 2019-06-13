
if(NOT ARCH)
    set(ARCH i386)
endif()

# Default to Debug for the build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif()

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
# clang-cl gets detected as "Clang" instead of "MSVC" so we force it here
if(USE_CLANG_CL)
    include(CMakeForceCompiler)
    CMAKE_FORCE_C_COMPILER(clang-cl MSVC)
    set(CMAKE_C_COMPILER_VERSION "16.00.40219.01")
    if(ARCH STREQUAL "i386")
        set(MSVC_C_ARCHITECTURE_ID "X86")
    endif()
    include(${CMAKE_ROOT}/Modules/CMakeClDeps.cmake)
else()
    set(CMAKE_C_COMPILER cl)
endif()

if(ARCH STREQUAL "arm")
    include(CMakeForceCompiler)
    CMAKE_FORCE_CXX_COMPILER(cl MSVC)
else()
    if(USE_CLANG_CL)
        include(CMakeForceCompiler)
        CMAKE_FORCE_CXX_COMPILER(clang-cl MSVC)
    else()
        set(CMAKE_CXX_COMPILER cl)
    endif()
endif()

set(CMAKE_MC_COMPILER mc)
set(CMAKE_RC_COMPILER rc)
if(ARCH STREQUAL "amd64")
    set(CMAKE_ASM_COMPILER ml64)
elseif(ARCH STREQUAL "arm")
    set(CMAKE_ASM_COMPILER armasm)
elseif(ARCH STREQUAL "arm64")
    set(CMAKE_ASM_COMPILER armasm64)
else()
    set(CMAKE_ASM_COMPILER ml)
endif()
set(CMAKE_ASM_COMPILER_ID "VISUAL")

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE INTERNAL "")

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    add_definitions(-D__i386__)
endif()

set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/overrides-msvc.cmake")
