
add_definitions(-DUSE_WINE_TODOS)

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
    precomp.h)

add_executable(ole32_winetest ${SOURCE} testlist.c)
target_link_libraries(ole32_winetest uuid)
set_module_type(ole32_winetest win32cui)
add_importlibs(ole32_winetest oleaut32 ole32 user32 gdi32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(ole32_winetest ntdll)
endif()

add_pch(ole32_winetest precomp.h SOURCE)
add_rostests_file(TARGET ole32_winetest)
