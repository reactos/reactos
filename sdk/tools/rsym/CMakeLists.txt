
include_directories(${REACTOS_SOURCE_DIR}/sdk/tools)
add_library(rsym_common STATIC rsym_common.c)
target_link_libraries(rsym_common PRIVATE host_includes)

if(ARCH STREQUAL "i386")
    add_host_tool(rsym rsym.c)
elseif(ARCH STREQUAL "amd64")
    add_host_tool(rsym rsym64.c)
elseif(ARCH STREQUAL "arm")
    add_executable(rsym rsym64.c)
elseif(ARCH STREQUAL "arm64")
    add_executable(rsym rsym64.c)
endif()

target_link_libraries(rsym PRIVATE host_includes rsym_common dbghelphost unicode)
add_host_tool(raddr2line raddr2line.c)
target_link_libraries(raddr2line PRIVATE host_includes rsym_common)
