
add_host_tool(hpp hpp.c)

if(MSVC)
    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    target_compile_options(hpp PRIVATE "/wd4244")
    
    if(ARCH STREQUAL "amd64")
        # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
        target_compile_options(hpp PRIVATE "/wd4267")
    endif()
endif()
