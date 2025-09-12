
macro(require_program varname execname)
    find_program(${varname} ${execname})
    if(NOT ${varname})
        message(FATAL_ERROR "${execname} not found")
    endif()
endmacro()

# pass variables necessary for the toolchain (needed for try_compile)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARCH)

# Choose the right MinGW toolchain prefix
if(NOT DEFINED MINGW_TOOLCHAIN_PREFIX)
    if(ARCH STREQUAL "i386")

        if(CMAKE_HOST_WIN32)
            set(MINGW_TOOLCHAIN_PREFIX "" CACHE STRING "MinGW Toolchain Prefix")
        else()
            set(MINGW_TOOLCHAIN_PREFIX "i686-w64-mingw32-" CACHE STRING "MinGW-W64 Toolchain Prefix")
        endif()

    elseif(ARCH STREQUAL "amd64")
        set(MINGW_TOOLCHAIN_PREFIX "x86_64-w64-mingw32-" CACHE STRING "MinGW Toolchain Prefix")
    elseif(ARCH STREQUAL "arm")
        set(MINGW_TOOLCHAIN_PREFIX "arm-mingw32ce-" CACHE STRING "MinGW Toolchain Prefix")
    endif()
endif()

if(NOT DEFINED MINGW_TOOLCHAIN_SUFFIX)
    set(MINGW_TOOLCHAIN_SUFFIX "" CACHE STRING "MinGW Toolchain Suffix")
endif()

# The name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Which tools to use
require_program(CMAKE_C_COMPILER ${MINGW_TOOLCHAIN_PREFIX}gcc${MINGW_TOOLCHAIN_SUFFIX})
require_program(CMAKE_CXX_COMPILER ${MINGW_TOOLCHAIN_PREFIX}g++${MINGW_TOOLCHAIN_SUFFIX})
require_program(CMAKE_ASM_COMPILER ${MINGW_TOOLCHAIN_PREFIX}gcc${MINGW_TOOLCHAIN_SUFFIX})
set(CMAKE_ASM_COMPILER_ID "GNU")
require_program(CMAKE_MC_COMPILER ${MINGW_TOOLCHAIN_PREFIX}windmc)
require_program(CMAKE_RC_COMPILER ${MINGW_TOOLCHAIN_PREFIX}windres)
require_program(CMAKE_DLLTOOL ${MINGW_TOOLCHAIN_PREFIX}dlltool)
#set(CMAKE_AR ${MINGW_TOOLCHAIN_PREFIX}gcc-ar${MINGW_TOOLCHAIN_SUFFIX})
require_program(CMAKE_OBJCOPY ${MINGW_TOOLCHAIN_PREFIX}objcopy)

# FIXME: On amd64, archives lose their index when re-archived by AR after being created by dlltool
# Use regular archives instead of thin archives (T flag) and always run ranlib
if(ARCH STREQUAL "amd64" OR ARCH STREQUAL "i386")
    # For amd64, we MUST run ranlib after every archive operation
    set(CMAKE_C_CREATE_STATIC_LIBRARY 
        "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>"
        "<CMAKE_RANLIB> <TARGET>")
    set(CMAKE_CXX_CREATE_STATIC_LIBRARY 
        "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>"
        "<CMAKE_RANLIB> <TARGET>")
    set(CMAKE_ASM_CREATE_STATIC_LIBRARY 
        "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>"
        "<CMAKE_RANLIB> <TARGET>")
    # FIXME: Also override the LINK rule for static libraries to ensure ranlib is called
    set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> r <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> <TARGET>")
    set(CMAKE_CXX_ARCHIVE_CREATE ${CMAKE_C_ARCHIVE_CREATE})
    set(CMAKE_CXX_ARCHIVE_APPEND ${CMAKE_C_ARCHIVE_APPEND})
    set(CMAKE_CXX_ARCHIVE_FINISH ${CMAKE_C_ARCHIVE_FINISH})
else()
    set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> crT <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})
    set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})
endif()

# Don't link with anything by default unless we say so
set(CMAKE_C_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C Libraries")

#MARK_AS_ADVANCED(CLEAR CMAKE_CXX_STANDARD_LIBRARIES)
set(CMAKE_CXX_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C++ Libraries")

# This allows to have CMake test the compiler without linking
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# AGENT-MODIFIED: Workaround for binutils 2.43.1 linker segmentation fault
# This version of binutils has a critical bug that causes segfaults when linking
# complex DLLs with many object files and symbol exports
# Applying multiple workarounds:
# 1. Disable auto-import to reduce symbol resolution complexity
# 2. Use response files for long command lines
# 3. Limit memory usage during linking
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-nostdlib -Wl,--disable-auto-import")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-nostdlib -Wl,--disable-auto-import")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -Wl,--disable-auto-import")

# Force use of response files for objects to work around command line length issues
set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS ON)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS ON)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES ON)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES ON)
set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")

set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/overrides-gcc.cmake")

# Get GCC version
execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
