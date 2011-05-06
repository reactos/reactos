
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    activex.c
    jscript.c
    run.c
    testlist.c
    rsrc.rc)

add_executable(jscript_winetest ${SOURCE})
target_link_libraries(jscript_winetest wine)
set_module_type(jscript_winetest win32cui)
add_importlibs(jscript_winetest ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_cab_target(jscript_winetest 7)
