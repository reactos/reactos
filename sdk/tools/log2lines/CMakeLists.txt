
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
    util.c)

include_directories(${REACTOS_SOURCE_DIR}/sdk/tools/rsym)
add_host_tool(log2lines ${SOURCE})
target_link_libraries(log2lines PRIVATE host_includes rsym_common)
