
cmake_minimum_required(VERSION 2.6)

if(POLICY CMP0017)
    # Shadow cmake provided modules
    cmake_policy(SET CMP0017 OLD)
endif()

project(REACTOS)

#versioning
include(include/reactos/version.cmake)

# Don't escape preprocessor definition values added via add_definitions
cmake_policy(SET CMP0005 OLD)
cmake_policy(SET CMP0002 NEW)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SKIP_PREPROCESSED_SOURCE_RULES TRUE)
set(CMAKE_SKIP_ASSEMBLY_SOURCE_RULES TRUE)
set(CMAKE_COLOR_MAKEFILE OFF)
#set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

if(NOT ARCH)
    set(ARCH i386)
endif()
string(TOLOWER ${ARCH} ARCH)

# Compile options
if(ARCH MATCHES i386)
    include(config.cmake)
elseif(ARCH MATCHES amd64)
    include(config-amd64.cmake)
elseif(ARCH MATCHES arm)
    include(config-arm.cmake)
endif()

add_definitions(-D__REACTOS__ -D__ROS_CMAKE__)

if(NOT CMAKE_CROSSCOMPILING)

    add_definitions(-DTARGET_${ARCH})

    if(MSVC)
        add_definitions(-Dinline=__inline)
    else()
       add_definitions(-fshort-wchar)
    endif()

    include_directories(
        ${REACTOS_SOURCE_DIR}/tools/unicode
        include
        include/host
        ${REACTOS_BINARY_DIR}/include)

    add_subdirectory(tools)
    add_subdirectory(lib)

    if(NOT MSVC)
        export(TARGETS widl wrc gendib cabman cdmake mkhive obj2bin spec2def geninc FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake NAMESPACE native- )
    else()
        export(TARGETS gendib cabman cdmake mkhive obj2bin spec2def geninc FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake NAMESPACE native- )
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
    endif()

    if(KDBG)
        add_definitions(-DKDBG=1)
    endif()

    if(_WINKD_)
        add_definitions(-D_WINKD_=1)
    endif()

    # Version Options
    add_definitions(-DWINVER=0x502
                    -D_WIN32_IE=0x600
                    -D_WIN32_WINNT=0x502
                    -D_WIN32_WINDOWS=0x502
                    -D_SETUPAPI_VER=0x502)

    # Arch Options
    if(ARCH MATCHES i386)
        add_definitions(-D_M_IX86 -D_X86_ -D__i386__)
    elseif(ARCH MATCHES amd64)
        add_definitions(-D_M_AMD64 -D_AMD64_ -D__x86_64__ -D_WIN64)
    elseif(ARCH MATCHES arm)
        # _M_ARM is already defined by toolchain
        add_definitions(-D_ARM_ -D__arm__)
    endif()

    # Other
    if(ARCH MATCHES i386)
        add_definitions(-DUSE_COMPILER_EXCEPTIONS -D_USE_32BIT_TIME_T)
    elseif(ARCH MATCHES amd64)
        add_definitions(-DUSE_COMPILER_EXCEPTIONS -DNO_UNDERSCORE_PREFIX)
    elseif(ARCH MATCHES arm)
        add_definitions(-DUSE_COMPILER_EXCEPTIONS)
    endif()

    # Compiler specific definitions and macros
    if(MSVC)
        include(msc.cmake)
    else()
        include(gcc.cmake)
    endif()

    # Generic macros
    include(CMakeMacros.cmake)

    # IDL macros for widl/midl
    include(cmake/idl-support.cmake)

    # Activate support for assembly source files
    enable_language(ASM)

    # Activate language support for resource files
    enable_language(RC)

    include_directories(
        include
        include/psdk
        include/dxsdk
        ${REACTOS_BINARY_DIR}/include
        ${REACTOS_BINARY_DIR}/include/dxsdk
        ${REACTOS_BINARY_DIR}/include/psdk
        ${REACTOS_BINARY_DIR}/include/reactos
        include/crt
        include/ddk
        include/ndk
        include/reactos
        include/reactos/libs)

    if(ARCH MATCHES arm)
        include_directories(${REACTOS_SOURCE_DIR}/include/reactos/arm)
    endif()

    if(MSVC)
        include_directories(include/crt/msc)
    else()
        include_directories(include/crt/mingw32)
    endif()

    add_dependency_header()

    add_subdirectory(include/psdk)
    add_subdirectory(include/dxsdk)
    add_subdirectory(include/reactos/idl)
    add_subdirectory(include/reactos/wine)
    add_subdirectory(include/reactos/mc)
    add_subdirectory(include/asm)

    include(baseaddress.cmake)

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

    file(MAKE_DIRECTORY ${REACTOS_BINARY_DIR}/include/reactos)

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/importlibs)

    add_dependency_footer()
endif()
