
include_directories(${REACTOS_SOURCE_DIR}/sdk/tools)
add_library(rsym_common STATIC rsym_common.c)
target_link_libraries(rsym_common PRIVATE host_includes)

add_host_tool(rsym rsym.c)

target_link_libraries(rsym PRIVATE host_includes rsym_common dbghelphost unicode)

if(MSVC AND ARCH STREQUAL "amd64")
    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
    target_compile_options(rsym PRIVATE "/wd4267")
endif()

add_host_tool(raddr2line raddr2line.c)
target_link_libraries(raddr2line PRIVATE host_includes rsym_common)
