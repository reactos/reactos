
list(APPEND SOURCE
    cache.c
    cmd.c
    help.c
    image.c
    list.c
    log2lines.c
    match.c
    options.c
    revision.c
    stat.c
    util.c)

include_directories(${REACTOS_SOURCE_DIR}/tools/rsym)
add_executable(log2lines ${SOURCE})
target_link_libraries(log2lines rsym_common)
