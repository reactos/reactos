
add_definitions(
    -DISOHYBRID_C_STANDALONE)

add_host_tool(isohybrid
    isohybrid.c
    reactos_support_code.c)
