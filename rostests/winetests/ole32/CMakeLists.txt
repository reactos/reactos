
list(APPEND SOURCE
    clipboard.c
    compobj.c
    defaulthandler.c
    dragdrop.c
    errorinfo.c
    hglobalstream.c
    marshal.c
    moniker.c
    ole2.c
    ole_server.c
    propvariant.c
    stg_prop.c
    storage32.c
    usrmarshal.c
    testlist.c)

add_executable(ole32_winetest ${SOURCE})
target_link_libraries(ole32_winetest uuid)
set_module_type(ole32_winetest win32cui)
add_importlibs(ole32_winetest oleaut32 ole32 user32 gdi32 advapi32 msvcrt kernel32)
add_cd_file(TARGET ole32_winetest DESTINATION reactos/bin FOR all)
