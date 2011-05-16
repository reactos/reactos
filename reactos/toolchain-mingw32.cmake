
if(NOT ARCH)
    set(ARCH i386)
endif()

# Choose the right MinGW prefix
if(ARCH MATCHES i386)

    if(CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
        set(MINGW_PREFIX "" CACHE STRING "MinGW Prefix")
    else()
        set(MINGW_PREFIX "mingw32-" CACHE STRING "MinGW Prefix")
    endif(CMAKE_HOST_SYSTEM_NAME MATCHES Windows)

elseif(ARCH MATCHES amd64)
    set(MINGW_PREFIX "x86_64-w64-mingw32-" CACHE STRING "MinGW Prefix")
elseif(ARCH MATCHES arm)
    set(MINGW_PREFIX "arm-mingw32ce-" CACHE STRING "MinGW Prefix")
endif()

if(ENABLE_CCACHE)
    set(CCACHE "ccache" CACHE STRING "ccache")
else()
    set(CCACHE "" CACHE STRING "ccache")
endif()

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER ${CCACHE} ${MINGW_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${CCACHE} ${MINGW_PREFIX}g++)
set(CMAKE_RC_COMPILER ${MINGW_PREFIX}windres)
set(CMAKE_ASM_COMPILER ${MINGW_PREFIX}gcc)
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -x assembler-with-cpp -o <OBJECT> -I${REACTOS_SOURCE_DIR}/include/asm -I${REACTOS_BINARY_DIR}/include/asm <FLAGS> <DEFINES> -D__ASM__ -c <SOURCE>")

# Use stdcall fixups, and don't link with anything by default unless we say so
set(CMAKE_C_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C Libraries")

#MARK_AS_ADVANCED(CLEAR CMAKE_CXX_STANDARD_LIBRARIES)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "Standard C++ Libraries")

if(ARCH MATCHES i386)
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-nodefaultlibs -nostdlib -Wl,--enable-auto-image-base -Wl,--disable-auto-import")
#-Wl,-T,${REACTOS_SOURCE_DIR}/global.lds
elseif(ARCH MATCHES amd64)
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-nodefaultlibs -nostdlib -Wl,--enable-auto-image-base -Wl,--disable-auto-import")
endif()
