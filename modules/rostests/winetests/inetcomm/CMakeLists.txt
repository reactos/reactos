
add_definitions(
    -DUSE_WINE_TODOS
    -Dstrcasecmp=_stricmp
)

list(APPEND SOURCE
    mimeintl.c
    mimeole.c
    testlist.c)

add_executable(inetcomm_winetest ${SOURCE})
set_module_type(inetcomm_winetest win32cui)
add_importlibs(inetcomm_winetest inetcomm oleaut32 ole32 urlmon msvcrt kernel32)
add_rostests_file(TARGET inetcomm_winetest)
