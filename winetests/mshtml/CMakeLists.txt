
set_rc_compiler()

add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    dom.c
    events.c
    htmldoc.c
    htmllocation.c
    misc.c
    protocol.c
    script.c
    testlist.c
    rsrc.rc)

add_executable(mshtml_winetest ${SOURCE})
target_link_libraries(mshtml_winetest wine uuid strmiids)
set_module_type(mshtml_winetest win32cui)
add_importlibs(mshtml_winetest wininet ole32 oleaut32 user32 gdi32 urlmon advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mshtml_winetest DESTINATION reactos/bin FOR all)
