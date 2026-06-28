
if(DEFINED ENV{_ROSBE_ROSSCRIPTDIR})
    set(CMAKE_SYSROOT $ENV{_ROSBE_ROSSCRIPTDIR}/$ENV{ROS_ARCH})
endif()

# pass variables necessary for the toolchain (needed for try_compile)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARCH CLANG_VERSION LLVM_MINGW_ROOT REACTOS_CLANG_REQUIRE_LLD)

# The name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)
# The processor we are targeting
if (ARCH STREQUAL "i386")
    set(CMAKE_SYSTEM_PROCESSOR i686)
elseif (ARCH STREQUAL "amd64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
elseif(ARCH STREQUAL "arm")
    set(CMAKE_SYSTEM_PROCESSOR arm)
else()
    message(FATAL_ERROR "Unsupported ARCH: ${ARCH}")
endif()

if (DEFINED CLANG_VERSION)
    set(CLANG_SUFFIX "-${CLANG_VERSION}")
else()
    set(CLANG_SUFFIX "")
endif()

# Which tools to use
set(triplet ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32)
if (CMAKE_HOST_WIN32)
    set(GCC_TOOLCHAIN_PREFIX "")
else()
    set(GCC_TOOLCHAIN_PREFIX "${triplet}-")
endif()

set(LLVM_MINGW_ROOT "" CACHE PATH "Path to an llvm-mingw toolchain root")
if(NOT LLVM_MINGW_ROOT AND DEFINED ENV{LLVM_MINGW_ROOT})
    set(LLVM_MINGW_ROOT "$ENV{LLVM_MINGW_ROOT}" CACHE PATH "Path to an llvm-mingw toolchain root" FORCE)
endif()

set(LLVM_MINGW_BIN "")
if(LLVM_MINGW_ROOT)
    get_filename_component(LLVM_MINGW_BIN "${LLVM_MINGW_ROOT}/bin" ABSOLUTE)
endif()

if(LLVM_MINGW_BIN)
    set(CMAKE_C_COMPILER "${LLVM_MINGW_BIN}/clang${CLANG_SUFFIX}")
else()
    set(CMAKE_C_COMPILER clang${CLANG_SUFFIX})
endif()
set(CMAKE_C_COMPILER_TARGET ${triplet})
if(LLVM_MINGW_BIN)
    set(CMAKE_CXX_COMPILER "${LLVM_MINGW_BIN}/clang++${CLANG_SUFFIX}")
else()
    set(CMAKE_CXX_COMPILER clang++${CLANG_SUFFIX})
endif()
set(CMAKE_CXX_COMPILER_TARGET ${triplet})
set(CMAKE_ASM_COMPILER ${GCC_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER_ID GNU)
set(CMAKE_MC_COMPILER ${GCC_TOOLCHAIN_PREFIX}windmc)
set(CMAKE_RC_COMPILER ${GCC_TOOLCHAIN_PREFIX}windres)
# set(CMAKE_AR ${triplet}-ar)

find_program(REACTOS_AS
    NAMES ${GCC_TOOLCHAIN_PREFIX}as
    PATHS /usr/bin /usr/local/bin
    NO_DEFAULT_PATH)
if(NOT REACTOS_AS)
    find_program(REACTOS_AS NAMES ${GCC_TOOLCHAIN_PREFIX}as)
endif()
if(NOT REACTOS_AS)
    message(FATAL_ERROR "GNU-compatible ${GCC_TOOLCHAIN_PREFIX}as not found")
endif()
execute_process(
    COMMAND ${REACTOS_AS} --version
    OUTPUT_VARIABLE REACTOS_AS_VERSION
    ERROR_VARIABLE REACTOS_AS_VERSION_ERROR)
string(FIND "${REACTOS_AS_VERSION}${REACTOS_AS_VERSION_ERROR}" "GNU assembler" REACTOS_AS_GNU_VERSION)
if(REACTOS_AS_GNU_VERSION EQUAL -1)
    message(FATAL_ERROR "${REACTOS_AS} is not GNU assembler")
endif()
get_filename_component(REACTOS_AS_DIR ${REACTOS_AS} DIRECTORY)
set(REACTOS_AS_PREFIX "${REACTOS_AS_DIR}/${GCC_TOOLCHAIN_PREFIX}")
set(REACTOS_AS_FLAGS "-B${REACTOS_AS_PREFIX}")
set(CMAKE_ASM_FLAGS_INIT "${REACTOS_AS_FLAGS}")
set(CMAKE_ASM_FLAGS "${REACTOS_AS_FLAGS}" CACHE STRING "GNU assembler handoff flags" FORCE)
message(STATUS "Using assembler ${REACTOS_AS}")

find_program(REACTOS_DLLTOOL
    NAMES ${triplet}-dlltool
    PATHS /usr/bin /usr/local/bin
    NO_DEFAULT_PATH)
if(NOT REACTOS_DLLTOOL)
    find_program(REACTOS_DLLTOOL NAMES ${triplet}-dlltool)
endif()
if(NOT REACTOS_DLLTOOL)
    message(FATAL_ERROR "GNU-compatible ${triplet}-dlltool not found")
endif()
execute_process(
    COMMAND ${REACTOS_DLLTOOL} --help
    OUTPUT_VARIABLE REACTOS_DLLTOOL_HELP
    ERROR_VARIABLE REACTOS_DLLTOOL_HELP_ERROR)
string(FIND "${REACTOS_DLLTOOL_HELP}${REACTOS_DLLTOOL_HELP_ERROR}" "--output-delaylib" REACTOS_DLLTOOL_DELAYLIB_FLAG)
if(REACTOS_DLLTOOL_DELAYLIB_FLAG EQUAL -1)
    message(FATAL_ERROR "${REACTOS_DLLTOOL} does not support --output-delaylib")
endif()
set(CMAKE_DLLTOOL ${REACTOS_DLLTOOL} CACHE FILEPATH "GNU-compatible dlltool" FORCE)
message(STATUS "Using dlltool ${CMAKE_DLLTOOL}")

# This allows to have CMake test the compiler without linking
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> crT <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})
set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})

set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "Standard C Libraries")
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "Standard C++ Libraries")

set(REACTOS_CLANG_REQUIRE_LLD OFF CACHE BOOL "Require an LLD linker for Clang MinGW builds")
if(LLVM_MINGW_BIN)
    set(REACTOS_CLANG_REQUIRE_LLD ON CACHE BOOL "Require an LLD linker for Clang MinGW builds" FORCE)
endif()

if(LLVM_MINGW_BIN)
    find_program(REACTOS_LD
        NAMES ld.lld ${GCC_TOOLCHAIN_PREFIX}ld
        PATHS "${LLVM_MINGW_BIN}"
        NO_DEFAULT_PATH)
endif()
if(NOT REACTOS_LD)
    find_program(REACTOS_LD
        NAMES ${GCC_TOOLCHAIN_PREFIX}ld
        PATHS /usr/bin /usr/local/bin
        NO_DEFAULT_PATH)
endif()
if(NOT REACTOS_LD)
    find_program(REACTOS_LD NAMES ${GCC_TOOLCHAIN_PREFIX}ld)
endif()
if(NOT REACTOS_LD)
    message(FATAL_ERROR "GNU-compatible ${GCC_TOOLCHAIN_PREFIX}ld not found")
endif()
execute_process(
    COMMAND ${REACTOS_LD} --version
    OUTPUT_VARIABLE REACTOS_LD_VERSION
    ERROR_VARIABLE REACTOS_LD_VERSION_ERROR)
string(FIND "${REACTOS_LD_VERSION}${REACTOS_LD_VERSION_ERROR}" "LLD" REACTOS_LD_LLD_VERSION)
if(REACTOS_LD_LLD_VERSION EQUAL -1)
    set(REACTOS_LD_IS_LLD FALSE CACHE INTERNAL "The Clang MinGW linker is LLD")
else()
    set(REACTOS_LD_IS_LLD TRUE CACHE INTERNAL "The Clang MinGW linker is LLD")
endif()
if(REACTOS_CLANG_REQUIRE_LLD)
    if(NOT REACTOS_LD_IS_LLD)
        message(FATAL_ERROR "${REACTOS_LD} is not LLD")
    endif()
endif()
execute_process(
    COMMAND ${REACTOS_LD} --help
    OUTPUT_VARIABLE REACTOS_LD_HELP
    ERROR_VARIABLE REACTOS_LD_HELP_ERROR)
string(FIND "${REACTOS_LD_HELP}${REACTOS_LD_HELP_ERROR}" "--script" REACTOS_LD_SCRIPT_FLAG)
if(REACTOS_LD_SCRIPT_FLAG EQUAL -1)
    message(FATAL_ERROR "${REACTOS_LD} does not support linker scripts")
endif()
set(CMAKE_LINKER ${REACTOS_LD} CACHE FILEPATH "GNU-compatible linker used by the Clang driver" FORCE)
set(LD_EXECUTABLE ${REACTOS_LD} CACHE FILEPATH "GNU-compatible linker used by ReactOS linker rules" FORCE)
message(STATUS "Using linker ${LD_EXECUTABLE}")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import -fuse-ld=${LD_EXECUTABLE}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import -fuse-ld=${LD_EXECUTABLE}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import -fuse-ld=${LD_EXECUTABLE}")

set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/overrides-gcc.cmake")
