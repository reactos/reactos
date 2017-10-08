
list(APPEND SOURCE
    container.c
    provider
    testlist.c)

add_executable(dxdiagn_winetest ${SOURCE})
set_module_type(dxdiagn_winetest win32cui)
add_importlibs(dxdiagn_winetest oleaut32 ole32 msvcrt kernel32)
add_rostests_file(TARGET dxdiagn_winetest)
