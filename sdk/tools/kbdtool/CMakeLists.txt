
add_host_tool(kbdtool data.c main.c output.c parser.c)
target_link_libraries(kbdtool PRIVATE host_includes)
