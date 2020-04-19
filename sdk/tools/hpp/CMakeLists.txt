
add_host_tool(hpp hpp.c)

if(MSVC)
    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    target_compile_options(hpp PRIVATE "/wd4244")
endif()
