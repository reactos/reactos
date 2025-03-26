
# pass variables necessary for the toolchain (needed for try_compile)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARCH USE_CLANG_CL)

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# set the generator platform
if (NOT DEFINED CMAKE_GENERATOR_PLATFORM)
    if(ARCH STREQUAL "amd64")
        set(CMAKE_GENERATOR_PLATFORM "x64")
    elseif(ARCH STREQUAL "arm")
        set(CMAKE_GENERATOR_PLATFORM "ARM")
    elseif(ARCH STREQUAL "arm64")
        set(CMAKE_GENERATOR_PLATFORM "ARM64")
    else()
        set(CMAKE_GENERATOR_PLATFORM "Win32")
    endif()
endif()

if(USE_CLANG_CL)
    set(CMAKE_C_COMPILER clang-cl)
    set(CMAKE_CXX_COMPILER clang-cl)
    # Clang now defaults to lld-link which we're not compatible with yet
    set(CMAKE_LINKER link)
    # llvm-lib with link.exe can't generate proper delayed imports
    set(CMAKE_AR lib)
    set(CMAKE_C_COMPILER_AR lib)
    set(CMAKE_CXX_COMPILER_AR lib)
    # Explicitly set target so CMake doesn't get confused
    if (ARCH STREQUAL "amd64")
        set(CMAKE_C_COMPILER_TARGET "x86_64-pc-windows-msvc")
        set(CMAKE_CXX_COMPILER_TARGET "x86_64-pc-windows-msvc")
    elseif(ARCH STREQUAL "arm")
        set(CMAKE_C_COMPILER_TARGET "arm-pc-windows-msvc")
        set(CMAKE_CXX_COMPILER_TARGET "arm-pc-windows-msvc")
    elseif(ARCH STREQUAL "arm64")
        set(CMAKE_C_COMPILER_TARGET "arm64-pc-windows-msvc")
        set(CMAKE_CXX_COMPILER_TARGET "arm64-pc-windows-msvc")
    else()
        # -m32 is required for x64 clang-cl to operate in x86 Native Tools environment
        set(CMAKE_C_FLAGS "-m32")
        set(CMAKE_CXX_FLAGS "-m32")
        set(CMAKE_C_COMPILER_TARGET "i686-pc-windows-msvc")
        set(CMAKE_CXX_COMPILER_TARGET "i686-pc-windows-msvc")
    endif()

    # Avoid wrapping RC compiler with cmcldeps utility for clang-cl.
    # Otherwise it breaks cross-compilation (32bit ReactOS cannot be compiled by 64bit LLVM),
    # target architecture is not passed properly
    set(CMAKE_NINJA_CMCLDEPS_RC OFF)
else()
    set(CMAKE_C_COMPILER cl)
    set(CMAKE_CXX_COMPILER cl)
endif()

set(CMAKE_MC_COMPILER mc)
set(CMAKE_RC_COMPILER rc)
if(ARCH STREQUAL "amd64")
    set(CMAKE_ASM_MASM_COMPILER ml64)
    set(CMAKE_ASM_MASM_FLAGS_INIT "/Cp")
elseif(ARCH STREQUAL "arm")
    set(CMAKE_ASM_MASM_COMPILER armasm)
elseif(ARCH STREQUAL "arm64")
    set(CMAKE_ASM_MASM_COMPILER armasm64)
else()
    set(CMAKE_ASM_MASM_COMPILER ml)
    set(CMAKE_ASM_MASM_FLAGS_INIT "/Cp")
endif()

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE INTERNAL "")

set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/overrides-msvc.cmake")
