
set(SARCH "pc" CACHE STRING
"Sub-architecture to build for. Specify one of:
 pc pc98 xbox")

set(OARCH "pentium" CACHE STRING
"Generate instructions for this CPU type. Specify one of:
 pentium, pentiumpro")

set(TUNE "i686" CACHE STRING
"Which CPU ReactOS should be optimized for.")

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

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(GCC TRUE CACHE BOOL "The compiler is GCC")
    set(CLANG FALSE CACHE BOOL "The compiler is LLVM Clang")
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(GCC FALSE CACHE BOOL "The compiler is GCC")
    set(CLANG TRUE CACHE BOOL "The compiler is LLVM Clang")
elseif(MSVC) # aka CMAKE_C_COMPILER_ID STEQUAL "MSVC"
    set(GCC FALSE CACHE BOOL "The compiler is GCC")
    set(CLANG FALSE CACHE BOOL "The compiler is LLVM Clang")
    # MSVC variable is already set by cmake
else()
    message("WARNING: the compiler has not been recognized")
endif()

if(MSVC AND (NOT USE_CLANG_CL))
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

set(_ELF_ FALSE CACHE BOOL
"Whether to compile support for ELF files.
Do not enable unless you know what you're doing.")

set(BUILD_MP TRUE CACHE BOOL
"Whether to build the multiprocessor versions of NTOSKRNL and HAL.")

set(GENERATE_DEPENDENCY_GRAPH FALSE CACHE BOOL
"Whether to create a GraphML dependency graph of DLLs.")

if(MSVC)
set(_PREFAST_ FALSE CACHE BOOL
"Whether to enable PREFAST while compiling.")
set(_VS_ANALYZE_ FALSE CACHE BOOL
"Whether to enable static analysis while compiling.")
    # RTC are incompatible with compiler optimizations.
    CMAKE_DEPENDENT_OPTION(RUNTIME_CHECKS "Whether to enable runtime checks on MSVC" ON
                           "CMAKE_BUILD_TYPE STREQUAL \"Debug\"" OFF)
endif()

if(GCC)
    option(STACK_PROTECTOR "Whether to enable the GCC stack checker while compiling" OFF)
endif()

set(USE_DUMMY_PSEH FALSE CACHE BOOL
"Whether to disable PSEH support.")
