
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

if(USE_CLANG_CL)
    set(CMAKE_C_COMPILER clang-cl)
    set(CMAKE_CXX_COMPILER clang-cl)
    # Clang now defaults to lld-link which we're not compatible with yet
    set(CMAKE_LINKER link)
else()
    set(CMAKE_C_COMPILER cl)
    set(CMAKE_CXX_COMPILER cl)
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

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE INTERNAL "")

set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/overrides-msvc.cmake")
