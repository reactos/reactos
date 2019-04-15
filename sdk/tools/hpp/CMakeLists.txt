
add_host_tool(hpp hpp.c)

if(MSVC)
    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    add_target_compile_flags(hpp "/wd4244")
endif()
