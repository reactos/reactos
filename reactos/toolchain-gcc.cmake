
if(NOT ARCH)
    set(ARCH i386)
endif()

# Choose the right MinGW toolchain prefix
if (NOT DEFINED MINGW_TOOLCHAIN_PREFIX)
    if(ARCH MATCHES i386)

        if(CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
            set(MINGW_TOOLCHAIN_PREFIX "" CACHE STRING "MinGW Toolchain Prefix")
        else()
            set(MINGW_TOOLCHAIN_PREFIX "mingw32-" CACHE STRING "MinGW Toolchain Prefix")
        endif(CMAKE_HOST_SYSTEM_NAME MATCHES Windows)

    elseif(ARCH MATCHES amd64)
        set(MINGW_TOOLCHAIN_PREFIX "x86_64-w64-mingw32-" CACHE STRING "MinGW Toolchain Prefix")
    elseif(ARCH MATCHES arm)
        set(MINGW_TOOLCHAIN_PREFIX "arm-mingw32ce-" CACHE STRING "MinGW Toolchain Prefix")
    endif()
endif()

if (NOT DEFINED MINGW_TOOLCHAIN_SUFFIX)
    set(MINGW_TOOLCHAIN_SUFFIX "" CACHE STRING "MinGW Toolchain Suffix")
endif()

if(ENABLE_CCACHE)
    set(CCACHE "ccache" CACHE STRING "ccache")
else()
    set(CCACHE "" CACHE STRING "ccache")
endif()

# The name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Which tools to use
set(CMAKE_C_COMPILER ${MINGW_TOOLCHAIN_PREFIX}gcc${MINGW_TOOLCHAIN_SUFFIX})
set(CMAKE_CXX_COMPILER ${MINGW_TOOLCHAIN_PREFIX}g++${MINGW_TOOLCHAIN_SUFFIX})
set(CMAKE_ASM_COMPILER ${MINGW_TOOLCHAIN_PREFIX}gcc${MINGW_TOOLCHAIN_SUFFIX})
set(CMAKE_ASM_COMPILER_ID "GNU")
set(CMAKE_MC_COMPILER ${MINGW_TOOLCHAIN_PREFIX}windmc)
set(CMAKE_RC_COMPILER ${MINGW_TOOLCHAIN_PREFIX}windres)
set(CMAKE_DLLTOOL ${MINGW_TOOLCHAIN_PREFIX}dlltool)

if(NOT CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
    set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})
    set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})
endif()

# Don't link with anything by default unless we say so
set(CMAKE_C_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C Libraries")

#MARK_AS_ADVANCED(CLEAR CMAKE_CXX_STANDARD_LIBRARIES)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "Standard C++ Libraries")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import")

# Get GCC version
execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
