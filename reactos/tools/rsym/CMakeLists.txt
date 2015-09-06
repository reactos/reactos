
include_directories(${REACTOS_SOURCE_DIR}/tools)
add_library(rsym_common rsym_common.c)

if(ARCH STREQUAL "i386")
    add_definitions(-D_X86_)
    add_host_tool(rsym rsym.c)
elseif(ARCH STREQUAL "amd64")
    add_host_tool(rsym rsym64.c)
elseif(ARCH STREQUAL "arm")
    add_executable(rsym rsym64.c)
endif()

target_link_libraries(rsym rsym_common dbghelphost zlibhost unicode)
add_host_tool(raddr2line rsym_common.c raddr2line.c)
