cmake_minimum_required(VERSION 3.8)

project(lwip C)

set (BUILD_SHARED_LIBS ON)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_SYSTEM_NAME STREQUAL "GNU")
    message(FATAL_ERROR "Lwip shared library is only working on Linux or the Hurd")
endif()

set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
include(${LWIP_DIR}/contrib/ports/CMakeCommon.cmake)

set (LWIP_DEFINITIONS -DLWIP_DEBUG)
set (LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${LWIP_CONTRIB_DIR}/"
    "${LWIP_CONTRIB_DIR}/ports/unix/port/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)

set (LWIP_EXCLUDE_SLIPIF TRUE)
include(${LWIP_CONTRIB_DIR}/ports/unix/Filelists.cmake)
include(${LWIP_DIR}/src/Filelists.cmake)

add_library(lwip ${lwipnoapps_SRCS} ${lwipcontribportunix_SRCS} ${lwipcontribportunixnetifs_SRCS})
target_compile_options(lwip PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwip PRIVATE ${LWIP_DEFINITIONS} ${LWIP_MBEDTLS_DEFINITIONS})
target_include_directories(lwip PRIVATE ${LWIP_INCLUDE_DIRS} ${LWIP_MBEDTLS_INCLUDE_DIRS})
target_link_libraries(lwip ${LWIP_SANITIZER_LIBS})

find_library(LIBPTHREAD pthread)
target_link_libraries(lwip ${LIBPTHREAD})
