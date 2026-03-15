macro(require_llvm_program varname execname)
    if(DEFINED CLANG_VERSION)
        find_program(${varname} NAMES ${execname}-${CLANG_VERSION} ${execname} HINTS ${_llvm_tool_bin_hints})
    elseif(DEFINED LLVM_TOOL_VERSION)
        find_program(${varname} NAMES ${execname}-${LLVM_TOOL_VERSION} ${execname} HINTS ${_llvm_tool_bin_hints})
    else()
        find_program(${varname} NAMES ${execname} HINTS ${_llvm_tool_bin_hints})
    endif()

    if(NOT ${varname})
        message(FATAL_ERROR "${execname} not found")
    endif()
endmacro()

function(_detect_llvm_mingw_root _outvar)
    set(_candidate_roots)
    if(DEFINED ENV{LLVM_MINGW_ROOT})
        list(APPEND _candidate_roots "$ENV{LLVM_MINGW_ROOT}")
    endif()
    if(DEFINED ENV{LLVM_MINGW_PREFIX})
        list(APPEND _candidate_roots "$ENV{LLVM_MINGW_PREFIX}")
    endif()
    if(DEFINED ENV{HOME})
        list(APPEND _candidate_roots "$ENV{HOME}/mingw-toolchains/llvm-mingw")
        file(GLOB _home_llvm_mingw_candidates LIST_DIRECTORIES TRUE "$ENV{HOME}/mingw-toolchains/llvm-mingw-*")
        list(APPEND _candidate_roots ${_home_llvm_mingw_candidates})
    endif()

    list(APPEND _candidate_roots "/opt/llvm-mingw")
    file(GLOB _llvm_mingw_candidates LIST_DIRECTORIES TRUE "/opt/llvm-mingw-*")
    list(APPEND _candidate_roots ${_llvm_mingw_candidates})
    list(REMOVE_DUPLICATES _candidate_roots)

    foreach(_candidate IN LISTS _candidate_roots)
        if(EXISTS "${_candidate}/bin/clang" AND EXISTS "${_candidate}/bin/clang++")
            set(${_outvar} "${_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${_outvar} "" PARENT_SCOPE)
endfunction()

function(_detect_rosbe_gcc_toolchain _outvar)
    if(ARCH STREQUAL "i386")
        set(_rosbe_gcc_dir gcc-i686)
    elseif(ARCH STREQUAL "amd64")
        set(_rosbe_gcc_dir gcc-x86_64)
    else()
        set(${_outvar} "" PARENT_SCOPE)
        return()
    endif()

    set(_candidate_roots)
    if(DEFINED ENV{_ROSBE_PREFIX})
        list(APPEND _candidate_roots "$ENV{_ROSBE_PREFIX}/${_rosbe_gcc_dir}")
    endif()

    list(APPEND _candidate_roots
        "/opt/rosbe-linux/${_rosbe_gcc_dir}"
        "${CMAKE_CURRENT_LIST_DIR}/../RosBE/RosBE-Linux/${_rosbe_gcc_dir}")

    file(GLOB _rosbe_build_candidates LIST_DIRECTORIES TRUE
        "${CMAKE_CURRENT_LIST_DIR}/../RosBE/RosBE-Linux/builddir/*/opt/rosbe-linux/${_rosbe_gcc_dir}")
    list(APPEND _candidate_roots ${_rosbe_build_candidates})

    foreach(_candidate IN LISTS _candidate_roots)
        if(EXISTS "${_candidate}/bin")
            set(${_outvar} "${_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${_outvar} "" PARENT_SCOPE)
endfunction()

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    ARCH
    CLANG_VERSION
    REACTOS_CLANG_GCC_TOOLCHAIN
    REACTOS_CLANG_LLVM_MINGW_ROOT)
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

set(triplet ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32)

if(NOT DEFINED REACTOS_CLANG_LLVM_MINGW_ROOT OR NOT REACTOS_CLANG_LLVM_MINGW_ROOT)
    _detect_llvm_mingw_root(_llvm_mingw_root)
    if(_llvm_mingw_root)
        set(REACTOS_CLANG_LLVM_MINGW_ROOT "${_llvm_mingw_root}" CACHE PATH
            "Preferred llvm-mingw toolchain root used to provide the native Clang Windows GNU sysroot")
    endif()
endif()

set(_llvm_tool_bin_hints)
if(REACTOS_CLANG_LLVM_MINGW_ROOT)
    list(APPEND _llvm_tool_bin_hints "${REACTOS_CLANG_LLVM_MINGW_ROOT}/bin")
endif()

find_program(_llvm_triplet_c_compiler NAMES ${triplet}-clang HINTS ${_llvm_tool_bin_hints} NO_DEFAULT_PATH)
find_program(_llvm_triplet_cxx_compiler NAMES ${triplet}-clang++ HINTS ${_llvm_tool_bin_hints} NO_DEFAULT_PATH)
if(_llvm_triplet_c_compiler)
    set(CMAKE_C_COMPILER "${_llvm_triplet_c_compiler}")
else()
    require_llvm_program(CMAKE_C_COMPILER clang)
endif()
if(_llvm_triplet_cxx_compiler)
    set(CMAKE_CXX_COMPILER "${_llvm_triplet_cxx_compiler}")
else()
    require_llvm_program(CMAKE_CXX_COMPILER clang++)
endif()
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

if(NOT DEFINED CLANG_VERSION)
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -dumpversion
        OUTPUT_VARIABLE _clang_version
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "^[0-9]+" LLVM_TOOL_VERSION "${_clang_version}")
endif()

if(NOT DEFINED REACTOS_CLANG_GCC_TOOLCHAIN OR NOT REACTOS_CLANG_GCC_TOOLCHAIN)
    _detect_rosbe_gcc_toolchain(_rosbe_gcc_toolchain)
    if(_rosbe_gcc_toolchain)
        set(REACTOS_CLANG_GCC_TOOLCHAIN "${_rosbe_gcc_toolchain}" CACHE PATH
            "Auxiliary MinGW-w64 GCC toolchain root used to provide runtime libraries for Clang builds")
    endif()
endif()

if(NOT DEFINED CMAKE_SYSROOT OR NOT CMAKE_SYSROOT)
    if(REACTOS_CLANG_LLVM_MINGW_ROOT AND EXISTS "${REACTOS_CLANG_LLVM_MINGW_ROOT}/${triplet}/lib")
        set(CMAKE_SYSROOT "${REACTOS_CLANG_LLVM_MINGW_ROOT}")
    elseif(DEFINED ENV{_ROSBE_ROSSCRIPTDIR})
        set(CMAKE_SYSROOT "$ENV{_ROSBE_ROSSCRIPTDIR}/$ENV{ROS_ARCH}")
    elseif(REACTOS_CLANG_GCC_TOOLCHAIN AND EXISTS "${REACTOS_CLANG_GCC_TOOLCHAIN}/${triplet}/sysroot")
        set(CMAKE_SYSROOT "${REACTOS_CLANG_GCC_TOOLCHAIN}/${triplet}/sysroot")
    endif()
endif()

set(CMAKE_C_COMPILER_TARGET ${triplet})
set(CMAKE_CXX_COMPILER_TARGET ${triplet})
set(CMAKE_ASM_COMPILER_TARGET ${triplet})
set(CMAKE_ASM_COMPILER_ID Clang)

set(_mingw_tool_bin_hints)
if(REACTOS_CLANG_GCC_TOOLCHAIN)
    list(APPEND _mingw_tool_bin_hints "${REACTOS_CLANG_GCC_TOOLCHAIN}/bin")
endif()

find_program(CMAKE_MC_COMPILER NAMES ${triplet}-windmc native-windmc windmc HINTS ${_mingw_tool_bin_hints} ${_llvm_tool_bin_hints})
if(NOT CMAKE_MC_COMPILER)
    message(FATAL_ERROR "${triplet}-windmc or native-windmc not found")
endif()
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
