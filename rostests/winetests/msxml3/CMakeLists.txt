
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    domdoc.c
    httpreq.c
    saxreader.c
    schema.c
    testlist.c
    xmldoc.c
    xmlparser.c
    xmlview.c)

add_executable(msxml3_winetest ${SOURCE} rsrc.rc)
add_idl_headers(xmlparser_idlheader_test xmlparser.idl)
add_dependencies(msxml3_winetest xmlparser_idlheader_test)
target_link_libraries(msxml3_winetest wine)
set_module_type(msxml3_winetest win32cui)
add_importlibs(msxml3_winetest user32 ole32 oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msxml3_winetest DESTINATION reactos/bin FOR all)
