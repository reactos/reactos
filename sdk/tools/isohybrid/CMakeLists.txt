
add_definitions(
    -DISOHYBRID_C_STANDALONE)

add_host_tool(isohybrid
    isohybrid.c
    reactos_support_code.c)

if(MSVC)
    # Disable warning "'x': unreferenced local variable"
    add_target_compile_flags(isohybrid "/wd4101")

    # Disable warning "'return': conversion from '__int64' to 'int', possible loss of data"
    add_target_compile_flags(isohybrid "/wd4244")
endif()
