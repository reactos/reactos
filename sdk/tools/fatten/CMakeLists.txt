
add_host_tool(fatten
    fatten.c
    fatfs/diskio.c
    fatfs/ff.c
    fatfs/option/ccsbcs.c)
target_link_libraries(fatten PRIVATE host_includes)

if(MSVC AND ARCH STREQUAL "amd64")
    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
    target_compile_options(fatten PRIVATE "/wd4267")
endif()
