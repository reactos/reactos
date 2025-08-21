
list(APPEND SOURCE
    cache.c
    cmd.c
    help.c
    image.c
    list.c
    log2lines.c
    match.c
    options.c
    stat.c
    util.c
    ../port/getopt.c)

include_directories(${REACTOS_SOURCE_DIR}/sdk/tools/rsym)
if(MSVC)
    include_directories(../port)
    
    if(ARCH STREQUAL "amd64")
        # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
        target_compile_options(log2lines PRIVATE "/wd4267")
    endif()
endif()
add_host_tool(log2lines ${SOURCE})
target_link_libraries(log2lines PRIVATE host_includes rsym_common)
