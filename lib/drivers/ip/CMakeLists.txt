add_definitions(-D__NTDRIVER__)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/drivers/network/tcpip/include)
include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/oskittcp/include)

file(GLOB_RECURSE ARCH_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/network/${ARCH}/checksum.S)
file(GLOB_RECURSE SOURCE "*.c")

add_library(ip ${ARCH_SOURCE} ${SOURCE})
add_dependencies(ip bugcodes)

