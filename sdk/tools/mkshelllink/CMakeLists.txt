
add_host_tool(mkshelllink mkshelllink.c)
if(MSVC)
    target_compile_options(mkshelllink PRIVATE /wd4815)
else()
    target_compile_options(mkshelllink PRIVATE "-fshort-wchar")
endif()
