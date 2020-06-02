
set(SARCH "omap3-zoom2" CACHE STRING
"Sub-architecture (board) to build for. Specify one of:
 kurobox versatile omap3-zoom2 omap3-beagle")

set(OARCH "armv7-a" CACHE STRING
"Generate instructions for this CPU type. Specify one of:
 armv5te armv7-a")

set(OPTIMIZE "1" CACHE STRING
"What level of optimization to use.
 0 = off
 1 = Default option, optimize for size (-Os) with some additional options
 2 = Optimize for size (-Os)
 3 = Optimize debugging experience (-Og)
 4 = Optimize (-O1)
 5 = Optimize even more (-O2)
 6 = Optimize yet more (-O3)
 7 = Disregard strict standards compliance (-Ofast)")

set(LTCG FALSE CACHE BOOL
"Whether to build with link-time code generation")

set(DBG TRUE CACHE BOOL
"Whether to compile for debugging.")

set(KDBG FALSE CACHE BOOL
"Whether to compile in the integrated kernel debugger.")

set(GDB FALSE CACHE BOOL
"Whether to compile for debugging with GDB.
If you don't use GDB, don't enable this.")

set(_WINKD_ TRUE CACHE BOOL
"Whether to compile with the KD protocol.")

set(_ELF_ FALSE CACHE BOOL
"Whether to compile support for ELF files.
Do not enable unless you know what you're doing.")

set(BUILD_MP TRUE CACHE BOOL
"Whether to compile the multi processor versions for ntoskrnl and hal.")

set(NEWSPRINTF FALSE CACHE BOOL
"Whether to compile the new sprintf.")

if(MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        option(RUNTIME_CHECKS "Whether to enable runtime checks on MSVC" ON)
    else()
        # RTC are incompatible with compiler optimizations.
        set(RUNTIME_CHECKS FALSE CACHE BOOL "Whether to enable runtime checks on MSVC")
    endif()
endif()
