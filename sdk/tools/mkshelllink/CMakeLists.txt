
add_host_tool(mkshelllink mkshelllink.c)
if(NOT MSVC)
    target_compile_options(mkshelllink PRIVATE "-fshort-wchar")
endif()
