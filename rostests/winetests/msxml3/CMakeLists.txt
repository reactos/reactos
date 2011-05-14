
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    domdoc.c
    saxreader.c
    schema.c
    testlist.c
    xmldoc.c
    xmlelem.c)

add_executable(msxml3_winetest ${SOURCE})
target_link_libraries(msxml3_winetest wine)
set_module_type(msxml3_winetest win32cui)
add_importlibs(msxml3_winetest user32 ole32 oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msxml3_winetest DESTINATION reactos/bin FOR all)
