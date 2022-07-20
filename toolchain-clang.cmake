
if(DEFINED ENV{_ROSBE_ROSSCRIPTDIR})
    set(CMAKE_SYSROOT $ENV{_ROSBE_ROSSCRIPTDIR}/$ENV{ROS_ARCH})
endif()

# pass variables necessary for the toolchain (needed for try_compile)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARCH CLANG_VERSION)

# The name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
# The processor we are targeting
if (ARCH STREQUAL "i386")
    set(CMAKE_SYSTEM_PROCESSOR i686)
elseif (ARCH STREQUAL "amd64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
elseif(ARCH STREQUAL "arm")
    set(CMAKE_SYSTEM_PROCESSOR arm)
else()
    message(FATAL_ERROR "Unsupported ARCH: ${ARCH}")
endif()

if (DEFINED CLANG_VERSION)
    set(CLANG_SUFFIX "-${CLANG_VERSION}")
else()
    set(CLANG_SUFFIX "")
endif()

# Which tools to use
set(triplet ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32)
if (CMAKE_HOST_WIN32)
    set(GCC_TOOLCHAIN_PREFIX "")
else()
    set(GCC_TOOLCHAIN_PREFIX "${triplet}-")
endif()

set(CMAKE_C_COMPILER clang${CLANG_SUFFIX})
set(CMAKE_C_COMPILER_TARGET ${triplet})
set(CMAKE_CXX_COMPILER clang++${CLANG_SUFFIX})
set(CMAKE_CXX_COMPILER_TARGET ${triplet})
set(CMAKE_ASM_COMPILER ${GCC_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER_ID GNU)
set(CMAKE_MC_COMPILER ${GCC_TOOLCHAIN_PREFIX}windmc)
set(CMAKE_RC_COMPILER ${GCC_TOOLCHAIN_PREFIX}windres)
# set(CMAKE_AR ${triplet}-ar)
# set(CMAKE_DLLTOOL ${triplet}-dlltool)

# This allows to have CMake test the compiler without linking
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> crT <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})
set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})

set(CMAKE_C_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C Libraries")
set(CMAKE_CXX_STANDARD_LIBRARIES "-lgcc" CACHE STRING "Standard C++ Libraries")

find_program (LD_EXECUTABLE ${GCC_TOOLCHAIN_PREFIX}ld)
message(STATUS "Using linker ${LD_EXECUTABLE}")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import -fuse-ld=${LD_EXECUTABLE}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import -fuse-ld=${LD_EXECUTABLE}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import -fuse-ld=${LD_EXECUTABLE}")

set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/overrides-gcc.cmake")
