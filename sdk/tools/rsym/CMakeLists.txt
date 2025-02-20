
include_directories(${REACTOS_SOURCE_DIR}/sdk/tools)
add_library(rsym_common STATIC rsym_common.c)
target_link_libraries(rsym_common PRIVATE host_includes)

add_host_tool(rsym rsym.c)

target_link_libraries(rsym PRIVATE host_includes rsym_common dbghelphost unicode)
add_host_tool(raddr2line raddr2line.c)
target_link_libraries(raddr2line PRIVATE host_includes rsym_common)
