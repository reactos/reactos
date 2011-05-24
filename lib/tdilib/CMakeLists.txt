
# FIXFIX: You guys should move this header in include/reactos/libs/iphlpapi!
include_directories(${REACTOS_SOURCE_DIR}/dll/win32/iphlpapi) 
add_library(tdilib enum.c handle.c)
add_dependencies(tdilib psdk)
