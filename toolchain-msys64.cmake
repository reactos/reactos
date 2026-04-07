
# Toolchain file for building ReactOS with MSYS2/ucrt64 on Windows.
# This is essentially toolchain-gcc.cmake but forces cross-compilation
# since the host and target are both Windows, which CMake normally
# treats as a native build.

macro(require_program varname execname)
    find_program(${varname} ${execname})
    if(NOT ${varname})
        message(FATAL_ERROR "${execname} not found")
    endif()
endmacro()

# pass variables necessary for the toolchain (needed for try_compile)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARCH)

# On MSYS2, tools are unprefixed (gcc, windmc, etc.)
if(NOT DEFINED MINGW_TOOLCHAIN_PREFIX)
    set(MINGW_TOOLCHAIN_PREFIX "" CACHE STRING "MinGW Toolchain Prefix")
endif()

if(NOT DEFINED MINGW_TOOLCHAIN_SUFFIX)
    set(MINGW_TOOLCHAIN_SUFFIX "" CACHE STRING "MinGW Toolchain Suffix")
endif()

# The name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Force cross-compilation even though host and target are both Windows.
# Without this, CMake sees host=Windows, target=Windows and skips the
# cross-compiling code path, which means ReactOS targets are never added.
set(CMAKE_CROSSCOMPILING TRUE)

# Which tools to use
require_program(CMAKE_C_COMPILER ${MINGW_TOOLCHAIN_PREFIX}gcc${MINGW_TOOLCHAIN_SUFFIX})
require_program(CMAKE_CXX_COMPILER ${MINGW_TOOLCHAIN_PREFIX}g++${MINGW_TOOLCHAIN_SUFFIX})
require_program(CMAKE_ASM_COMPILER ${MINGW_TOOLCHAIN_PREFIX}gcc${MINGW_TOOLCHAIN_SUFFIX})
set(CMAKE_ASM_COMPILER_ID "GNU")
require_program(CMAKE_MC_COMPILER ${MINGW_TOOLCHAIN_PREFIX}windmc)
require_program(CMAKE_RC_COMPILER ${MINGW_TOOLCHAIN_PREFIX}windres)
require_program(CMAKE_DLLTOOL ${MINGW_TOOLCHAIN_PREFIX}dlltool)
require_program(CMAKE_OBJCOPY ${MINGW_TOOLCHAIN_PREFIX}objcopy)

set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> crT <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})
set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})

# Don't link with anything by default unless we say so
set(CMAKE_C_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C Libraries")
set(CMAKE_CXX_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C++ Libraries")

# This allows to have CMake test the compiler without linking
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_SHARED_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import")

set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/overrides-gcc.cmake")

# Get GCC version
execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
