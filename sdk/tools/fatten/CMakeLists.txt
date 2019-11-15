
add_host_tool(fatten
    fatten.c
    fatfs/diskio.c
    fatfs/ff.c
    fatfs/option/ccsbcs.c)
target_link_libraries(fatten PRIVATE host_includes)
