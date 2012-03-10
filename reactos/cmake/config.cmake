
set(SARCH "pc" CACHE STRING
"Sub-architecture to build for. Specify one of: xbox")

set(OARCH "pentium" CACHE STRING
"Generate instructions for this CPU type. Specify one of:
 native, i386, i486, pentium, pentium-mmx, pentiumpro, i686,
 pentium2, pentium3, pentium-m, pentium4, prescott, nocona,
 core2, k6, k6-2, athlon, athlon-xp, opteron, opteron-sse3,
 barcelona, winchip-c6, winchip2, c3, c3-2, geode")

set(TUNE "i686" CACHE STRING
"Which CPU ReactOS should be optimized for.")

set(OPTIMIZE "1" CACHE STRING
"What level of optimisation to use.
  0 = off
  1 = Default option, optimize for size (-Os) with some additional options
  2 = -Os
  3 = -O1
  4 = -O2
  5 = -O3")

set(LTCG FALSE CACHE BOOL
"Whether to build with link-time code generation")

set(GDB FALSE CACHE BOOL
"Whether to compile for debugging with GDB.
If you don't use GDB, don't	enable this.")

if(${CMAKE_BUILD_TYPE} MATCHES Release)
    set(DBG FALSE CACHE BOOL
"Whether to compile for debugging.")
else()
    set(DBG TRUE CACHE BOOL
"Whether to compile for debugging.")
endif()

if(MSVC)
    set(KDBG FALSE CACHE BOOL
"Whether to compile in the integrated kernel debugger.")
    if(${CMAKE_BUILD_TYPE} MATCHES Release)
        set(_WINKD_ FALSE CACHE BOOL "Whether to compile with the KD protocol.")
    else()
        set(_WINKD_ TRUE CACHE BOOL "Whether to compile with the KD protocol.")
    endif()
    
else()
    set(KDBG TRUE CACHE BOOL
"Whether to compile in the integrated kernel debugger.")
    set(_WINKD_ FALSE CACHE BOOL
"Whether to compile with the KD protocol.")
endif()

set(_ELF_ FALSE CACHE BOOL
"Whether to compile support for ELF files.
Do not enable unless you know what you're doing.")

set(NSWPAT FALSE CACHE BOOL
"Whether to compile apps/libs with features covered software patents or not.
If you live in a country where software patents are valid/apply, don't
enable this (except they/you purchased a license from the patent owner).
This settings is disabled (0) by default.")

set(BUILD_MP TRUE CACHE BOOL
"Whether to compile the multi processor versions for ntoskrnl and hal.")

set(GENERATE_DEPENDENCY_GRAPH FALSE CACHE BOOL
"Whether to create a graphml dependency of dlls.")

if(MSVC)
set(_PREFAST_ FALSE CACHE BOOL
"Whether to enable PREFAST while compiling.")
endif()
