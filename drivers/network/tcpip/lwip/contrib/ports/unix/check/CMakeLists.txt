cmake_minimum_required(VERSION 3.8)

set (CMAKE_CONFIGURATION_TYPES "Debug;Release")

project(lwipunittests C)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_SYSTEM_NAME STREQUAL "GNU")
    message(FATAL_ERROR "Unit test are currently only working on Linux, Darwin or Hurd")
endif()

set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
if (NOT CMAKE_SYSTEM_NAME STREQUAL "GNU")
    set(LWIP_USE_SANITIZERS true)
endif()
include(${LWIP_DIR}/contrib/ports/CMakeCommon.cmake)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # check.h causes 'error: token pasting of ',' and __VA_ARGS__ is a GNU extension' with clang 9.0.0
    list(LWIP_COMPILER_FLAGS APPEND -Wno-gnu-zero-variadic-macro-arguments)
endif()

set (LWIP_DEFINITIONS -DLWIP_DEBUG -DLWIP_NOASSERT_ON_ERROR)
set (LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/test/unit"
    "${LWIP_DIR}/src/include"
    "${LWIP_CONTRIB_DIR}/"
    "${LWIP_CONTRIB_DIR}/ports/unix/port/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)

include(${LWIP_CONTRIB_DIR}/ports/unix/Filelists.cmake)
include(${LWIP_DIR}/src/Filelists.cmake)
include(${LWIP_DIR}/test/unit/Filelists.cmake)

add_executable(lwip_unittests ${LWIP_TESTFILES})
target_include_directories(lwip_unittests PRIVATE ${LWIP_INCLUDE_DIRS})
target_compile_options(lwip_unittests PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwip_unittests PRIVATE ${LWIP_DEFINITIONS} ${LWIP_MBEDTLS_DEFINITIONS})

find_library(LIBCHECK check)
find_library(LIBM m)
target_link_libraries(lwip_unittests ${LWIP_SANITIZER_LIBS} lwipallapps lwipcore ${LIBCHECK} ${LIBM})

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # check installed via brew on Darwin doesn't have a separate subunit library (must be statically linked)
    find_library(LIBSUBUNIT subunit)
    target_link_libraries(lwip_unittests ${LIBSUBUNIT})
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "GNU")
    find_library(LIBUTIL util)
    find_library(LIBPTHREAD pthread)
    find_library(LIBRT rt)
    target_link_libraries(lwip_unittests ${LIBUTIL} ${LIBPTHREAD} ${LIBRT})
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Darwin doesn't have pthreads or POSIX real-time extensions libs
    find_library(LIBUTIL util)
    target_link_libraries(lwip_unittests ${LIBUTIL})
endif()
