
list(APPEND SOURCE
    asmpp.cpp
)

add_host_tool(asmpp ${SOURCE})
target_link_libraries(asmpp PRIVATE host_includes)
set_property(TARGET asmpp PROPERTY CXX_STANDARD 11)
