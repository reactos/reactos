
cmake_minimum_required(VERSION 3.2.1)
cmake_policy(VERSION 3.2.1)

if(NOT CMAKE_VERSION MATCHES "ReactOS")
    message(WARNING "Building with \"${CMAKE_COMMAND}\", which is not the custom CMake included in RosBE, might cause build issues...")
endif()

if(POLICY CMP0058)
    # Ninja requires custom command byproducts to be explicit
    cmake_policy(SET CMP0058 OLD)
endif()

if(POLICY CMP0065)
    # Do not add flags to export symbols from executables without the ENABLE_EXPORTS target property
    cmake_policy(SET CMP0065 NEW)
endif()

include(CMakeDependentOption)

project(REACTOS)

# Versioning
include(sdk/include/reactos/version.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES TRUE)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES TRUE)
set(CMAKE_COLOR_MAKEFILE OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
#set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

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
    include(sdk/cmake/config.cmake)
elseif(ARCH STREQUAL "amd64")
    include(sdk/cmake/config-amd64.cmake)
elseif(ARCH STREQUAL "arm")
    include(sdk/cmake/config-arm.cmake)
endif()

# Compiler flags handling
include(sdk/cmake/compilerflags.cmake)

add_definitions(-D__REACTOS__)

# Double escape, since CMake unescapes before putting it on the command-line, where it's unescaped again by GCC/CL.
add_definitions(-DREACTOS_SOURCE_DIR="${REACTOS_SOURCE_DIR}")
add_definitions(-DREACTOS_BINARY_DIR="${REACTOS_BINARY_DIR}")

# There doesn't seem to be a standard for __FILE__ being relative or absolute, so detect it at runtime.
file(RELATIVE_PATH _PATH_PREFIX ${REACTOS_BINARY_DIR} ${REACTOS_SOURCE_DIR})
add_compile_flags(-D__RELFILE__="&__FILE__[__FILE__[0] == '.' ? sizeof \\\"${_PATH_PREFIX}\\\" - 1 : sizeof REACTOS_SOURCE_DIR]")

if(MSVC_IDE)
    add_compile_flags("/MP")
endif()

# Bison and Flex support
# include(sdk/cmake/bison-flex.cmake)

if(NOT CMAKE_CROSSCOMPILING)
    set(TOOLS_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
    add_definitions(-DTARGET_${ARCH})

    if(MSVC)
        if(ARCH STREQUAL "i386")
            add_definitions(/D_X86_ /D__i386__ /DWIN32 /D_WINDOWS)
        elseif(ARCH STREQUAL "amd64")
            add_definitions(-D_AMD64_ -D__x86_64__ /DWIN32 -D_WINDOWS)
        endif()
        if(MSVC_VERSION GREATER 1699)
            add_definitions(/D_ALLOW_KEYWORD_MACROS)
        endif()
        if(NOT USE_CLANG_CL)
            # FIXME: Inspect
            add_definitions(/Dinline=__inline)
        endif()
    endif()
    add_subdirectory(sdk/include/host)

    if(NOT MSVC)
        add_subdirectory(dll/win32/dbghelp)
    endif()
    add_subdirectory(sdk/tools)
    add_subdirectory(sdk/lib)

    set(NATIVE_TARGETS bin2c widl gendib cabman fatten hpp isohybrid mkhive mkisofs obj2bin spec2def geninc mkshelllink utf16le xml2sdb)
    if(NOT MSVC)
        list(APPEND NATIVE_TARGETS rsym pefixup)
    endif()

    export(TARGETS ${NATIVE_TARGETS} FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake NAMESPACE native- )
    configure_file(sdk/cmake/host-tools.in ${CMAKE_BINARY_DIR}/TargetList.cmake)

else()
    # Add host tools target
    include(sdk/cmake/host-tools.cmake)
    setup_host_tools()

    # We don't need CMake importlib handling.
    unset(CMAKE_IMPORT_LIBRARY_SUFFIX)

    # Print build type
    message("-- Build Type: ${CMAKE_BUILD_TYPE}")

    # adjust the default behaviour of the FIND_XXX() commands:
    # search headers and libraries in the target environment, search
    # programs in the host environment
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

    #useful stuff!
    include(CMakeParseArguments)

    if(DBG)
        add_definitions(-DDBG=1 -D_SEH_ENABLE_TRACE)
    else()
        add_definitions(-DDBG=0)
    endif()

    if(KDBG)
        add_definitions(-DKDBG)
    endif()

    if(_WINKD_)
        add_definitions(-D_WINKD_)
    endif()

    if(CMAKE_VERSION MATCHES "ReactOS" AND CMAKE_VERSION VERSION_LESS 3.3.0)
        set(PCH 1 CACHE BOOL "Whether to use precompiled headers")
    else()
        set(PCH 0 CACHE BOOL "Whether to use precompiled headers")
    endif()

    # Version Options
    add_definitions(-DWINVER=0x502
                    -D_WIN32_IE=0x600
                    -D_WIN32_WINNT=0x502
                    -D_WIN32_WINDOWS=0x502
                    -D_SETUPAPI_VER=0x502
                    -DMINGW_HAS_SECURE_API=1)

    # Arch Options
    if(ARCH STREQUAL "i386")
        if(NOT USE_CLANG_CL)
            add_definitions(-D_M_IX86)
        endif()
        add_definitions(-D_X86_ -D__i386__ -Di386)
        if(SARCH STREQUAL "xbox")
            add_definitions(-DSARCH_XBOX)
        elseif(SARCH STREQUAL "pc98")
            add_definitions(-DSARCH_PC98)
        endif()
    elseif(ARCH STREQUAL "amd64")
        add_definitions(-D_M_AMD64 -D_AMD64_ -D__x86_64__ -D_WIN64)
    elseif(ARCH STREQUAL "arm")
        # _M_ARM is already defined by toolchain
        add_definitions(-D_ARM_ -D__arm__ -DWIN32)
        if(SARCH STREQUAL "omap-zoom2")
            add_definitions(-D_ZOOM2_)
        endif()
    endif()

    # Other
    add_definitions(-D_NEW_DELETE_OPERATORS_)
    if(ARCH STREQUAL "i386")
        add_definitions(-DUSE_COMPILER_EXCEPTIONS -D_USE_32BIT_TIME_T)
    elseif(ARCH STREQUAL "amd64")
        add_definitions(-DUSE_COMPILER_EXCEPTIONS -DNO_UNDERSCORE_PREFIX)
    elseif(ARCH STREQUAL "arm")
        add_definitions(-DUSE_COMPILER_EXCEPTIONS -DNO_UNDERSCORE_PREFIX)
    endif()

    # Activate support for assembly source files
    enable_language(ASM)

    # Activate language support for resource files
    enable_language(RC)

    # Localization definitions
    include(sdk/cmake/localization.cmake)
    set(I18N_DEFS "")
    # This will set I18N_DEFS for later use
    set_i18n_language(${I18N_LANG})

    # Compiler specific definitions and macros
    if(MSVC)
        include(sdk/cmake/msvc.cmake)
    else()
        include(sdk/cmake/gcc.cmake)
    endif()

    # Generic macros
    include(sdk/cmake/CMakeMacros.cmake)

    # IDL macros for widl/midl
    # We're using widl now for both MSVC and GCC builds
    include(sdk/cmake/widl-support.cmake)

    include_directories(
        sdk/include
        sdk/include/psdk
        sdk/include/dxsdk
        ${REACTOS_BINARY_DIR}/sdk/include
        ${REACTOS_BINARY_DIR}/sdk/include/psdk
        ${REACTOS_BINARY_DIR}/sdk/include/dxsdk
        ${REACTOS_BINARY_DIR}/sdk/include/ddk
        ${REACTOS_BINARY_DIR}/sdk/include/reactos
        ${REACTOS_BINARY_DIR}/sdk/include/reactos/mc
        sdk/include/crt
        sdk/include/ddk
        sdk/include/ndk
        sdk/include/reactos
        sdk/include/reactos/libs)

    if(ARCH STREQUAL "arm")
        include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/arm)
    endif()

    add_dependency_header()

    add_subdirectory(sdk/include/ndk/tests)
    add_subdirectory(sdk/include/xdk)
    add_subdirectory(sdk/include/psdk)
    add_subdirectory(sdk/include/dxsdk)
    add_subdirectory(sdk/include/reactos/wine)
    add_subdirectory(sdk/include/reactos/mc)
    add_subdirectory(sdk/include/asm)

    if(NO_ROSSYM)
        include(sdk/cmake/baseaddress_dwarf.cmake)
    elseif(MSVC)
        include(sdk/cmake/baseaddress_msvc.cmake)
    else()
        include(sdk/cmake/baseaddress.cmake)
    endif()

    # For MSVC builds, this puts all debug symbols file in the same directory.
    if(MSVC)
        set(CMAKE_PDB_OUTPUT_DIRECTORY "${REACTOS_BINARY_DIR}/msvc_pdb")
    elseif(SEPARATE_DBG)
        set(CMAKE_PDB_OUTPUT_DIRECTORY "${REACTOS_BINARY_DIR}/symbols")
    endif()

    #begin with boot so reactos_cab target is defined before all other modules
    add_subdirectory(boot)
    add_subdirectory(base)
    add_subdirectory(dll)
    add_subdirectory(drivers)
    add_subdirectory(hal)
    add_subdirectory(sdk/lib)
    add_subdirectory(media)
    add_subdirectory(modules)
    add_subdirectory(ntoskrnl)
    add_subdirectory(subsystems)
    add_subdirectory(sdk/tools/wpp)
    add_subdirectory(win32ss)

    # Create the registry hives
    create_registry_hives()

    # Create {bootcd, livecd, bootcdregtest}.lst
    create_iso_lists()

    file(MAKE_DIRECTORY ${REACTOS_BINARY_DIR}/sdk/include/reactos)

    add_dependency_footer()
endif()
