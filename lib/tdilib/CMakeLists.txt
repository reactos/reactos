# FIXFIX: You guys should move this header in include/reactos/libs/iphlpapi!
include_directories(${REACTOS_SOURCE_DIR}/dll/win32/iphlpapi) 

file(GLOB_RECURSE SOURCE "*.c")

add_library(tdilib ${SOURCE})