
list(APPEND SOURCE
    asmpp.cpp
)

add_host_tool(asmpp ${SOURCE})
target_link_libraries(asmpp PRIVATE host_includes)
set_property(TARGET asmpp PROPERTY CXX_STANDARD 11)

if(MSVC AND ARCH STREQUAL "amd64")
    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
    target_compile_options(asmpp PRIVATE "/wd4267")
endif()
