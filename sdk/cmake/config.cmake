
if(ARCH STREQUAL "i386")
    set(SARCH "pc" CACHE STRING
    "Sub-architecture to build for. Specify one of:
     pc pc98 xbox")
elseif(ARCH STREQUAL "amd64")
    set(SARCH "" CACHE STRING
    "Sub-architecture to build for.")
elseif(ARCH STREQUAL "arm")
    set(SARCH "omap3-zoom2" CACHE STRING
    "Sub-architecture (board) to build for. Specify one of:
     kurobox versatile omap3-zoom2 omap3-beagle")
elseif(ARCH STREQUAL "arm64")
    # By design, arm64 kernels and OSes should be intercompatible, but
    # due to SoC vendors seemingly not being able to follow ARM design guidelines
    # properly, there might be a need for board-specific builds later on...
    set(SARCH "" CACHE STRING
    "Sub-architecture (board) to build for.")
endif()

if(ARCH STREQUAL "i386")
    set(OARCH "pentium" CACHE STRING
    "Generate instructions for this CPU type. Specify one of:
     pentium, pentiumpro")
elseif(ARCH STREQUAL "amd64")
    set(OARCH "athlon64" CACHE STRING
    "Generate instructions for this CPU type. Specify one of:
     k8 opteron athlon64 athlon-fx")
elseif(ARCH STREQUAL "arm")
    set(OARCH "armv7-a" CACHE STRING
    "Generate instructions for this CPU type. Specify one of:
     armv5te armv7-a")
elseif(ARCH STREQUAL "arm64")
    # This should not be bumped unless REALLY needed, because (as of 2021)
    # there are still new designs using the original A53 cores w/ armv8.0.
    set(OARCH "armv8-a" CACHE STRING
    "Generate instructions for this CPU type. Specify one of:
     armv8-a armv8.1-a armv8.2-a armv8.3-a armv8.4-a armv8.5-a armv8.6-a")
endif()

if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
    set(TUNE "generic" CACHE STRING
    "Which CPU ReactOS should be optimized for.")
elseif(ARCH STREQUAL "arm")
    set(TUNE "generic-armv7-a" CACHE STRING
    "Which CPU ReactOS should be optimized for.")
elseif(ARCH STREQUAL "arm64")
    set(TUNE "generic" CACHE STRING
    "Which CPU ReactOS should be optimized for.")
endif()

set(OPTIMIZE "4" CACHE STRING
"What level of optimization to use.
 0 = Off
 1 = Optimize for size (-Os) with some additional options
 2 = Optimize for size (-Os)
 3 = Optimize debugging experience (-Og)
 4 = Optimize (-O1)
 5 = Optimize even more (-O2)
 6 = Optimize yet more (-O3)
 7 = Disregard strict standards compliance (-Ofast)")

set(LTCG FALSE CACHE BOOL
"Whether to build with link-time code generation")

set(GDB FALSE CACHE BOOL
"Whether to compile for debugging with GDB.
If you don't use GDB, don't enable this.")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(DBG FALSE CACHE BOOL
"Whether to compile for debugging.")
else()
    set(DBG TRUE CACHE BOOL
"Whether to compile for debugging.")
endif()

if(MSVC)
    set(KDBG FALSE CACHE BOOL
"Whether to compile in the integrated kernel debugger.")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_WINKD_ FALSE CACHE BOOL "Whether to compile with the KD protocol.")
    else()
        set(_WINKD_ TRUE CACHE BOOL "Whether to compile with the KD protocol.")
    endif()
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(KDBG FALSE CACHE BOOL "Whether to compile in the integrated kernel debugger.")
    else()
        set(KDBG TRUE CACHE BOOL "Whether to compile in the integrated kernel debugger.")
    endif()
    set(_WINKD_ FALSE CACHE BOOL "Whether to compile with the KD protocol.")
endif()

option(BUILD_MP "Whether to build the multiprocessor versions of NTOSKRNL and HAL." ON)

cmake_dependent_option(ISAPNP_ENABLE "Whether to enable the ISA PnP support." ON
                       "ARCH STREQUAL i386 AND NOT SARCH STREQUAL xbox" OFF)

set(GENERATE_DEPENDENCY_GRAPH FALSE CACHE BOOL
"Whether to create a GraphML dependency graph of DLLs.")

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    option(_PREFAST_ "Whether to enable PREFAST while compiling." OFF)
    option(_VS_ANALYZE_ "Whether to enable static analysis while compiling." OFF)
    # RTC are incompatible with compiler optimizations.
    cmake_dependent_option(RUNTIME_CHECKS "Whether to enable runtime checks on MSVC" ON
                           "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    option(STACK_PROTECTOR "Whether to enable the GCC stack checker while compiling" OFF)
endif()

set(USE_DUMMY_PSEH FALSE CACHE BOOL
"Whether to disable PSEH support.")

set(DLL_EXPORT_VERSION "0x502" CACHE STRING
"The NT version the user mode DLLs target.")
