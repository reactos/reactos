
cmake_minimum_required(VERSION 2.8.12)

if(POLICY CMP0017)
    # Shadow cmake provided modules
    cmake_policy(SET CMP0017 OLD)
endif()

if(POLICY CMP0026)
    # Allow use of the LOCATION property
    cmake_policy(SET CMP0026 OLD)
endif()

project(REACTOS)

# Versioning
include(include/reactos/version.cmake)

# Don't escape preprocessor definition values added via add_definitions
cmake_policy(SET CMP0005 OLD)
cmake_policy(SET CMP0002 NEW)
if(POLICY CMP0018)
    cmake_policy(SET CMP0018 OLD)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES TRUE)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES TRUE)
set(CMAKE_COLOR_MAKEFILE OFF)
#set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)
set(CMAKE_SKIP_INSTALL_RULES ON)

if(NOT CMAKE_VERSION STREQUAL "2.8.12.1-ReactOS")
    set(CMAKE_DISABLE_NINJA_DEPSLOG TRUE)
endif()

if(NOT ARCH)
    set(ARCH i386)
endif()
# Now the ARCH variable will be in lowercase.
# It is needed because STREQUAL comparison
# is case-sensitive.
# See http://cmake.3232098.n2.nabble.com/Case-insensitive-string-compare-td7580269.html
# for more information.
string(TOLOWER ${ARCH} ARCH)

# Compile options
if(ARCH STREQUAL "i386")
    include(cmake/config.cmake)
elseif(ARCH STREQUAL "amd64")
    include(cmake/config-amd64.cmake)
elseif(ARCH STREQUAL "arm")
    include(cmake/config-arm.cmake)
endif()

# Compiler flags handling
include(cmake/compilerflags.cmake)

add_definitions(-D__REACTOS__)

if(NOT CMAKE_CROSSCOMPILING)

    add_definitions(-DTARGET_${ARCH})

    if(MSVC)
        if(ARCH STREQUAL "i386")
            add_definitions(/D_X86_ /DWIN32 /D_WINDOWS)
        endif()
        if(MSVC_VERSION GREATER 1699)
            add_definitions(/D_ALLOW_KEYWORD_MACROS)
        endif()
        add_definitions(/Dinline=__inline)
    else()
        add_compile_flags("-fshort-wchar -Wno-multichar")
    endif()

    include_directories(include/host)

    if(NOT MSVC)
        add_subdirectory(dll/win32/dbghelp)
    endif()
    add_subdirectory(tools)
    add_subdirectory(lib)

    if(NOT MSVC)
        export(TARGETS bin2c cat widl gendib cabman cdmake mkhive obj2bin spec2def geninc rsym mkshelllink utf16le FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake NAMESPACE native- )
    else()
        export(TARGETS bin2c cat widl gendib cabman cdmake mkhive obj2bin spec2def geninc mkshelllink utf16le FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake NAMESPACE native- )
    endif()

else()

    # adjust the default behaviour of the FIND_XXX() commands:
    # search headers and libraries in the target environment, search
    # programs in the host environment
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

    #useful stuff!
    include(CMakeParseArguments)

    # Default to Debug for the build type
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
"Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)

    # Do some cleanup
    file(REMOVE
        ${REACTOS_BINARY_DIR}/dependencies.graphml
        ${REACTOS_BINARY_DIR}/boot/ros_livecd.txt
        ${REACTOS_BINARY_DIR}/boot/ros_livecd_target.txt
        ${REACTOS_BINARY_DIR}/boot/ros_minicd.txt
        ${REACTOS_BINARY_DIR}/boot/ros_minicd_target.txt
        ${REACTOS_BINARY_DIR}/boot/ros_cab.txt
        ${REACTOS_BINARY_DIR}/boot/ros_cab_target.txt)

    if(NOT DEFINED REACTOS_BUILD_TOOLS_DIR)
        set(REACTOS_BUILD_TOOLS_DIR ${REACTOS_SOURCE_DIR}/build)
    endif()

    set(IMPORT_EXECUTABLES "${REACTOS_BUILD_TOOLS_DIR}/ImportExecutables.cmake" CACHE FILEPATH "Host executables")
    include(${IMPORT_EXECUTABLES})

    if(DBG)
        add_definitions(-DDBG=1 -D_SEH_ENABLE_TRACE)
    else()
        add_definitions(-DDBG=0)
    endif()

    if(KDBG)
        add_definitions(-DKDBG=1)
    endif()

    if(_WINKD_)
        add_definitions(-D_WINKD_=1)
    endif()

    if(CMAKE_VERSION MATCHES "ReactOS")
        set(PCH 1 CACHE BOOL "Whether to use precompiled headers")
    else()
        set(PCH 0 CACHE BOOL "Whether to use precompiled headers")
    endif()

    # Version Options
    add_definitions(-DWINVER=0x502
                    -D_WIN32_IE=0x600
                    -D_WIN32_WINNT=0x502
                    -D_WIN32_WINDOWS=0x502
                    -D_SETUPAPI_VER=0x502)

    # Arch Options
    if(ARCH STREQUAL "i386")
        add_definitions(-D_M_IX86 -D_X86_ -D__i386__ -Di386)
    elseif(ARCH STREQUAL "amd64")
        add_definitions(-D_M_AMD64 -D_AMD64_ -D__x86_64__ -D_WIN64)
    elseif(ARCH STREQUAL "arm")
        # _M_ARM is already defined by toolchain
        add_definitions(-D_ARM_ -D__arm__)
    endif()

    # Other
    if(ARCH STREQUAL "i386")
        add_definitions(-DUSE_COMPILER_EXCEPTIONS -D_USE_32BIT_TIME_T)
    elseif(ARCH STREQUAL "amd64")
        add_definitions(-DUSE_COMPILER_EXCEPTIONS -DNO_UNDERSCORE_PREFIX)
    elseif(ARCH STREQUAL "arm")
        add_definitions(-DUSE_COMPILER_EXCEPTIONS)
    endif()

    # Activate support for assembly source files
    enable_language(ASM)

    # Activate language support for resource files
    enable_language(RC)

    # Localization definitions
    include(cmake/localization.cmake)
    set(I18N_DEFS "")
    # This will set I18N_DEFS for later use
    set_i18n_language(${I18N_LANG})

    # Compiler specific definitions and macros
    if(MSVC)
        include(cmake/msvc.cmake)
    else()
        include(cmake/gcc.cmake)
    endif()

    # Generic macros
    include(cmake/CMakeMacros.cmake)

    # IDL macros for widl/midl
    # We're using widl now for both MSVC and GCC builds
    include(cmake/widl-support.cmake)

    include_directories(
        include
        include/psdk
        include/dxsdk
        ${REACTOS_BINARY_DIR}/include
        ${REACTOS_BINARY_DIR}/include/psdk
        ${REACTOS_BINARY_DIR}/include/dxsdk
        ${REACTOS_BINARY_DIR}/include/reactos
        include/crt
        include/ddk
        include/ndk
        include/reactos
        include/reactos/libs)

    if(ARCH STREQUAL "arm")
        include_directories(${REACTOS_SOURCE_DIR}/include/reactos/arm)
    endif()

    add_dependency_header()

    add_subdirectory(include/psdk)
    add_subdirectory(include/dxsdk)
    add_subdirectory(include/reactos/wine)
    add_subdirectory(include/reactos/mc)
    add_subdirectory(include/asm)

    if(NO_ROSSYM)
        include(cmake/baseaddress_dwarf.cmake)
    else()
        include(cmake/baseaddress.cmake)
    endif()

    # For MSVC builds, this puts all debug symbols file in the same directory.
    set(CMAKE_PDB_OUTPUT_DIRECTORY "${REACTOS_BINARY_DIR}/msvc_pdb")

    #begin with boot so reactos_cab target is defined before all other modules
    add_subdirectory(boot)
    add_subdirectory(base)
    add_subdirectory(dll)
    add_subdirectory(drivers)
    add_subdirectory(hal)
    add_subdirectory(lib)
    add_subdirectory(media)
    add_subdirectory(modules)
    add_subdirectory(ntoskrnl)
    add_subdirectory(subsystems)
    add_subdirectory(tools/wpp)
    add_subdirectory(win32ss)

    # Create the registry hives
    create_registry_hives()

    # Create {bootcd, livecd, bootcdregtest}.lst
    create_iso_lists()

    file(MAKE_DIRECTORY ${REACTOS_BINARY_DIR}/include/reactos)

    add_dependency_footer()
endif()
