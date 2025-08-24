
add_host_tool(kbdtool data.c main.c output.c parser.c)
target_link_libraries(kbdtool PRIVATE host_includes)

if(MSVC AND ARCH STREQUAL "amd64")
    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
    target_compile_options(kbdtool PRIVATE "/wd4267")
endif()
