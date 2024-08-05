cmake_minimum_required(VERSION 3.8)
project(liblwip
    VERSION 2.2.0
    DESCRIPTION "lwip library for linux"
    HOMEPAGE_URL "http://wiki.virtualsquare.org"
    LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFile)

set (BUILD_SHARED_LIBS ON)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Lwip shared library is only working on Linux or the Hurd")
endif()

set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)

find_library(LIB_VDEPLUGOK vdeplug)
check_include_file(libvdeplug.h INCLUDE_VDEPLUGOK)
if (LIB_VDEPLUGOK AND INCLUDE_VDEPLUGOK)
	message(STATUS "Found libvdeplug: vde support added")
else()
	message(STATUS "Libvdeplug needs to be installed to add vde support")
endif()

include(${LWIP_DIR}/contrib/ports/CMakeCommon.cmake)

set (LWIP_DEFINITIONS -DLWIP_DEBUG)
set (LWIP_INCLUDE_DIRS
    "include"
    "${LWIP_DIR}/src/include"
    "${LWIP_CONTRIB_DIR}/"
    "${LWIP_CONTRIB_DIR}/ports/unix/port/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)

set (LWIP_EXCLUDE_SLIPIF TRUE)
include(${LWIP_CONTRIB_DIR}/ports/unix/Filelists.cmake)
include(${LWIP_DIR}/src/Filelists.cmake)
if (LIB_VDEPLUGOK AND INCLUDE_VDEPLUGOK)
set(lwipnoapps_SRCS ${lwipnoapps_SRCS}
   ${LWIP_DIR}/contrib/ports/unix/port/netif/vdeif.c)
endif()

add_library(lwip ${lwipnoapps_SRCS} ${lwipcontribportunix_SRCS} ${lwipcontribportunixnetifs_SRCS})
target_compile_options(lwip PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwip PRIVATE ${LWIP_DEFINITIONS} ${LWIP_MBEDTLS_DEFINITIONS})
target_include_directories(lwip PRIVATE ${LWIP_INCLUDE_DIRS} ${LWIP_MBEDTLS_INCLUDE_DIRS})
if (LIB_VDEPLUGOK AND INCLUDE_VDEPLUGOK)
   target_link_libraries(lwip ${LWIP_SANITIZER_LIBS} vdeplug)
else()
   target_link_libraries(lwip ${LWIP_SANITIZER_LIBS})
endif()

find_library(LIBPTHREAD pthread)
target_link_libraries(lwip ${LIBPTHREAD})

install(TARGETS lwip 
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set_target_properties(lwip PROPERTIES VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR})

install(DIRECTORY "${LWIP_DIR}/src/include/lwip"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lwip"
		FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY "${LWIP_DIR}/src/include/netif"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lwip"
		FILES_MATCHING PATTERN "*.h"
)

install(FILES lwipopts.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lwip")

install(DIRECTORY "${LWIP_DIR}/contrib/ports/unix/port/include/arch"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lwip"
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY "${LWIP_DIR}/contrib/ports/unix/port/include/netif"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lwip"
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY "${LWIP_DIR}/contrib/ports/unix/linuxlib/include/posix"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lwip"
    FILES_MATCHING PATTERN "*.h"
)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/Uninstall.cmake")

