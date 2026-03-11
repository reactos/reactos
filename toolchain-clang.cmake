macro(require_llvm_program varname execname)
    if(DEFINED CLANG_VERSION)
        find_program(${varname} NAMES ${execname}-${CLANG_VERSION} ${execname})
    elseif(DEFINED LLVM_TOOL_VERSION)
        find_program(${varname} NAMES ${execname}-${LLVM_TOOL_VERSION} ${execname})
    else()
        find_program(${varname} NAMES ${execname})
    endif()

    if(NOT ${varname})
        message(FATAL_ERROR "${execname} not found")
    endif()
endmacro()

if(DEFINED ENV{_ROSBE_ROSSCRIPTDIR})
    set(CMAKE_SYSROOT $ENV{_ROSBE_ROSSCRIPTDIR}/$ENV{ROS_ARCH})
endif()

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARCH CLANG_VERSION)
set(CMAKE_SYSTEM_NAME Windows)

if(ARCH STREQUAL "i386")
    set(CMAKE_SYSTEM_PROCESSOR i686)
elseif(ARCH STREQUAL "amd64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
elseif(ARCH STREQUAL "arm")
    set(CMAKE_SYSTEM_PROCESSOR arm)
elseif(ARCH STREQUAL "arm64")
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
else()
    message(FATAL_ERROR "Unsupported ARCH: ${ARCH}")
endif()

require_llvm_program(CMAKE_C_COMPILER clang)
require_llvm_program(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

if(NOT DEFINED CLANG_VERSION)
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -dumpversion
        OUTPUT_VARIABLE _clang_version
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "^[0-9]+" LLVM_TOOL_VERSION "${_clang_version}")
endif()

set(triplet ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32)

set(CMAKE_C_COMPILER_TARGET ${triplet})
set(CMAKE_CXX_COMPILER_TARGET ${triplet})
set(CMAKE_ASM_COMPILER_TARGET ${triplet})
set(CMAKE_ASM_COMPILER_ID Clang)

set(CMAKE_MC_COMPILER native-windmc)
require_llvm_program(CMAKE_RC_COMPILER llvm-windres)
require_llvm_program(CMAKE_AR llvm-ar)
require_llvm_program(CMAKE_DLLTOOL llvm-dlltool)
require_llvm_program(CMAKE_STRIP llvm-strip)
require_llvm_program(CMAKE_OBJCOPY llvm-objcopy)
require_llvm_program(CMAKE_LINKER ld.lld)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> crT <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})
set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "Standard C Libraries")
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "Standard C++ Libraries")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "-nostdlib -fuse-ld=lld -Wl,--enable-auto-image-base,--disable-auto-import")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-nostdlib -fuse-ld=lld -Wl,--enable-auto-image-base,--disable-auto-import")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -fuse-ld=lld -Wl,--enable-auto-image-base,--disable-auto-import")

set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/overrides-gcc.cmake")
