
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    bitmap.c
    bmpformat.c
    converter.c
    gifformat.c
    icoformat.c
    info.c
    metadata.c
    palette.c
    pngformat.c
    propertybag.c
    stream.c
    testlist.c
    tiffformat.c)

add_executable(windowscodecs_winetest ${SOURCE})
set_module_type(windowscodecs_winetest win32cui)
add_importlibs(windowscodecs_winetest windowscodecs oleaut32 ole32 user32 gdi32 shlwapi msvcrt kernel32)

if(MSVC)
    add_importlibs(windowscodecs_winetest ntdll)
endif()

add_rostests_file(TARGET windowscodecs_winetest)
