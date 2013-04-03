
add_definitions(-D__ROS_LONG64__)
list(APPEND SOURCE
    activex.c
    caller.c
    jscript.c
    run.c
    testlist.c
    rsrc.rc)

add_executable(jscript_winetest ${SOURCE})
target_link_libraries(jscript_winetest wine)
set_module_type(jscript_winetest win32cui)
add_importlibs(jscript_winetest ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET jscript_winetest DESTINATION reactos/bin FOR all)
