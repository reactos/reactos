
add_definitions(
    -DISOHYBRID_C_STANDALONE)

add_host_tool(isohybrid
    isohybrid.c
    reactos_support_code.c
    ../port/getopt.c
    ../port/getopt1.c)

if(MSVC AND ARCH STREQUAL "amd64")
    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
    target_compile_options(isohybrid PRIVATE "/wd4267")
endif()
