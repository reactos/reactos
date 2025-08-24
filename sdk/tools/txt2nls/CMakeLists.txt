
list(APPEND SOURCE main.cpp)

add_host_tool(txt2nls ${SOURCE})

if(MSVC AND ARCH STREQUAL "amd64")
    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"  
    target_compile_options(txt2nls PRIVATE "/wd4267")
endif()
