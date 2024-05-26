
cmake_minimum_required(VERSION 3.17.0)

if(NOT CMAKE_VERSION MATCHES "ReactOS")
    message(WARNING "Building with \"${CMAKE_COMMAND}\", which is not the custom CMake included in RosBE, might cause build issues...")
endif()

include(CMakeDependentOption)

# CMAKE_CROSSCOMPILING and MSVC_IDE are not set until project() is called, so let's test this instead
if ((DEFINED CMAKE_TOOLCHAIN_FILE) AND (CMAKE_GENERATOR MATCHES "Visual Studio.*"))
# Do not use MSVC_RUNTIME_LIBRARY target property. We use our own flags instead
message(WARNING "Setting policy CMP0091 to OLD behaviour")
cmake_policy(SET CMP0091 OLD)
endif()

project(REACTOS)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES TRUE)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES TRUE)
set(CMAKE_COLOR_MAKEFILE OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
#set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

# check that the ARCH (target architecture) variable is defined
if(NOT ARCH)
    message(FATAL_ERROR "Target architecture (ARCH) is not defined. Please, choose one of: i386, amd64, arm, arm64")
endif()
# Now the ARCH variable will be in lowercase.
# It is needed because STREQUAL comparison
# is case-sensitive.
# See http://cmake.3232098.n2.nabble.com/Case-insensitive-string-compare-td7580269.html
# for more information.
string(TOLOWER ${ARCH} ARCH)

# set possible values for cmake GUI
set_property(CACHE ARCH PROPERTY STRINGS "i386" "amd64" "arm" "arm64")

# Alternative WinNT-compatible architecture string
if(ARCH STREQUAL "i386")
    set(WINARCH "x86")
else()
    set(WINARCH ${ARCH})
endif()

# set CMAKE_BUILD_TYPE if not set
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to Debug as none was specified.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE
        STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Versioning
include(sdk/include/reactos/version.cmake)

# Compile options
include(sdk/cmake/config.cmake)

# Compiler flags handling
include(sdk/cmake/compilerflags.cmake)

add_definitions(
    -D__REACTOS__
    # swprintf without count argument is used in most of the codebase
    -D_CRT_NON_CONFORMING_SWPRINTFS
)

# There doesn't seem to be a standard for __FILE__ being relative or absolute, so detect it at runtime.
file(RELATIVE_PATH _PATH_PREFIX ${REACTOS_BINARY_DIR} ${REACTOS_SOURCE_DIR})
if (NOT MSVC AND ((CMAKE_C_COMPILER_ID STREQUAL "GNU") AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "8.0.0")
               OR (CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0.0")))
    # Thankfully, GCC has this
    add_compile_options(-ffile-prefix-map=${REACTOS_SOURCE_DIR}=)
    add_compile_options(-ffile-prefix-map=${_PATH_PREFIX}=)
else()
    string(LENGTH ${_PATH_PREFIX} _PATH_PREFIX_LENGTH)
    string(LENGTH ${REACTOS_SOURCE_DIR} REACTOS_SOURCE_DIR_LENGTH)
    math(EXPR REACTOS_SOURCE_DIR_LENGTH "${REACTOS_SOURCE_DIR_LENGTH} + 1")
    add_compile_definitions("$<$<COMPILE_LANGUAGE:C,CXX>:__RELFILE__=&__FILE__[__FILE__[0] == '.' ? ${_PATH_PREFIX_LENGTH} : ${REACTOS_SOURCE_DIR_LENGTH}]>")
endif()

if(MSVC_IDE)
    add_compile_options("/MP")
endif()

# Bison and Flex support
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

if(MSVC_IDE)
    # Bison needs M4 and BISON_PKGDATADIR set at build time,
    # but visual studio is hardly ever opened from the configure-time environment.
    # Since cmake does not support setting env variables for a custom command,
    # we have to write a wrapper that sets the variables and then executes bison.
    # Idea taken from https://stackoverflow.com/a/35032051/4928207
    if(DEFINED ENV{M4})
        # Store this environment variable for configure re-runs from withing visual studio.
        SET(ROS_SAVED_M4 "$ENV{M4}" CACHE INTERNAL "")
    endif()
    if(DEFINED ENV{BISON_PKGDATADIR})
        SET(ROS_SAVED_BISON_PKGDATADIR "$ENV{BISON_PKGDATADIR}" CACHE INTERNAL "")
    endif()

    # Tell the user about a misconfigured environment
    if("x${ROS_SAVED_M4}x" STREQUAL "xx" OR "x${ROS_SAVED_BISON_PKGDATADIR}x" STREQUAL "xx")
        message(FATAL_ERROR "\nM4 or BISON_PKGDATADIR environment variables not set, cannot continue!\n"
            "See https://reactos.org/wiki/Visual_Studio for more information!")
    endif()

    file(WRITE "${CMAKE_BINARY_DIR}/bison_wrapper.cmd"
                "@ECHO OFF\n"
                "set M4=${ROS_SAVED_M4}\n"
                "set BISON_PKGDATADIR=${ROS_SAVED_BISON_PKGDATADIR}\n"
                "${BISON_EXECUTABLE} %*\n")
    set(BISON_EXECUTABLE "${CMAKE_BINARY_DIR}/bison_wrapper.cmd")
    # And the same hacks for FLEX
    file(WRITE "${CMAKE_BINARY_DIR}/flex_wrapper.cmd"
                "@ECHO OFF\n"
                "set M4=${ROS_SAVED_M4}\n"
                "set BISON_PKGDATADIR=${ROS_SAVED_BISON_PKGDATADIR}\n"
                "${FLEX_EXECUTABLE} %*\n")
    set(FLEX_EXECUTABLE "${CMAKE_BINARY_DIR}/flex_wrapper.cmd")
endif()

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
    endif()
    add_subdirectory(sdk/include/host)

    add_subdirectory(dll/win32/dbghelp)
    add_subdirectory(sdk/tools)
    add_subdirectory(sdk/lib)

    set(NATIVE_TARGETS asmpp bin2c widl gendib cabman fatten hpp isohybrid mkhive mkisofs obj2bin spec2def geninc mkshelllink utf16le xml2sdb)
    if(NOT MSVC)
        list(APPEND NATIVE_TARGETS rsym pefixup)
    endif()

    install(TARGETS ${NATIVE_TARGETS})
else()
    # Add host tools target
    include(sdk/cmake/host-tools.cmake)
    setup_host_tools()

    # We don't need CMake importlib handling.
    unset(CMAKE_IMPORT_LIBRARY_SUFFIX)

    # Print build type(s)
    if(CMAKE_CONFIGURATION_TYPES)
        # Multi-config generators, like Visual Studio (MSBuild).
        message("-- Configuration types: ${CMAKE_CONFIGURATION_TYPES}")
    else()
        # Single-configuration generators, like Ninja.
        message("-- Build type: ${CMAKE_BUILD_TYPE}")
    endif()

    # Always add /MT in VS CMAKE_GENERATOR and define _SBCS otherwise VS thinks it's a multi-byte or whatever project
    if (MSVC_IDE)
        add_compile_options("/MT")
        add_compile_definitions(_SBCS)
    endif()


    # adjust the default behaviour of the FIND_XXX() commands:
    # search headers and libraries in the target environment, search
    # programs in the host environment
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

    # Add our own target properties
    # General module definitions
    define_property(TARGET PROPERTY REACTOS_MODULE_TYPE
        BRIEF_DOCS "The type of this module"
        FULL_DOCS [[
The type of this module.
One of "nativecui", "nativedll", "kernelmodedriver", "wdmdriver", "kerneldll", "win32cui", "win32gui", "win32dll", "win32ocx", "cpl" or "module"]])

    # C++
    define_property(TARGET PROPERTY WITH_CXX_EXCEPTIONS
        BRIEF_DOCS "Enable C++ exceptions on this target"
        FULL_DOCS [[
Enables C++ exception handling.
Enable this if the module uses try/catch or throw. You might also need this if you use a standard operator new (the one without nothrow).]])
    define_property(TARGET PROPERTY WITH_CXX_RTTI
        BRIEF_DOCS "Enable C++ RTTI on this target"
        FULL_DOCS [[
Enables run-time type information.
Enable this if the module uses typeid or dynamic_cast. You will probably need to link yith cpprt as well, if you are not already using STL.]])


    if(DBG)
        add_definitions(-DDBG=1 -D_SEH_ENABLE_TRACE)
    else()
        add_definitions(-DDBG=0)
    endif()

    if(ENABLE_CCACHE)
        message(WARNING "-- Disabling precompiled headers support (ccache).")
        option(PCH "Whether to use precompiled headers" OFF)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(WARNING "-- Disabling precompiled headers on GCC by default CORE-17108.")
        option(PCH "Whether to use precompiled headers" OFF)
    else()
        option(PCH "Whether to use precompiled headers" ON)
    endif()

    # Version Options
    add_definitions(-DWINVER=0x502
                    -D_WIN32_IE=0x600
                    -D_WIN32_WINNT=0x502
                    -D_WIN32_WINDOWS=0x502
                    -D_SETUPAPI_VER=0x502
                    -DMINGW_HAS_SECURE_API=1
                    -DD3D_UMD_INTERFACE_VERSION=0x000C # Vista
                    -DDXGKDDI_INTERFACE_VERSION=0x1052 # Vista
                    -DDLL_EXPORT_VERSION=${DLL_EXPORT_VERSION})

    # Arch Options
    if(ARCH STREQUAL "i386")
        # clang-cl defines this one for itself
        if(NOT (MSVC AND CMAKE_C_COMPILER_ID STREQUAL "Clang"))
            add_definitions(-D_M_IX86)
        endif()
        add_definitions(-D_X86_ -D__i386__ -Di386)
        if(SARCH STREQUAL "xbox")
            add_definitions(-DSARCH_XBOX)
        elseif(SARCH STREQUAL "pc98")
            add_definitions(-DSARCH_PC98)
        endif()
    elseif(ARCH STREQUAL "amd64")
        # clang-cl defines this one for itself
        if (NOT (MSVC AND CMAKE_C_COMPILER_ID STREQUAL "Clang"))
            add_compile_definitions(_M_AMD64)
        endif()
        add_definitions(-D_AMD64_ -D__x86_64__ -D_WIN64)
    elseif(ARCH STREQUAL "arm")
        # _M_ARM is already defined by toolchain
        add_definitions(-D_ARM_ -D__arm__ -DWIN32)
        if(SARCH STREQUAL "omap3-zoom2")
            add_definitions(-D_ZOOM2_)
        endif()
    elseif(ARCH STREQUAL "arm64")
        # GNU tools refer to arm64 as aarch64
        add_definitions(-D_ARM64_ -D__arm64__ -D__aarch64__ -D_WIN64)
    endif()

    # Other
    add_definitions(-D_NEW_DELETE_OPERATORS_)
    if(ARCH STREQUAL "i386")
        add_definitions(-DUSE_COMPILER_EXCEPTIONS -D_USE_32BIT_TIME_T)
    elseif(ARCH STREQUAL "amd64")
        add_compile_definitions(USE_COMPILER_EXCEPTIONS)
    elseif(ARCH STREQUAL "arm")
        add_compile_definitions(USE_COMPILER_EXCEPTIONS)
    elseif(ARCH STREQUAL "arm64")
        add_compile_definitions(USE_COMPILER_EXCEPTIONS)
    endif()

    # Activate support for assembly source files
    if (MSVC)
        enable_language(ASM_MASM)
    else()
        enable_language(ASM)
    endif()

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
        sdk/include/winrt
        ${REACTOS_BINARY_DIR}/sdk/include
        ${REACTOS_BINARY_DIR}/sdk/include/psdk
        ${REACTOS_BINARY_DIR}/sdk/include/dxsdk
        ${REACTOS_BINARY_DIR}/sdk/include/ddk
        ${REACTOS_BINARY_DIR}/sdk/include/reactos
        ${REACTOS_BINARY_DIR}/sdk/include/reactos/mc
        sdk/include/crt
        sdk/include/vcruntime
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

    if(ARCH MATCHES "64$")
        include(sdk/cmake/baseaddress64.cmake)  
    elseif(NO_ROSSYM)
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
