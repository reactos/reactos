
add_definitions(
    -DUSE_WINE_TODOS
    -DWINETEST_USE_DBGSTR_LONGLONG)

list(APPEND SOURCE
    bitmap.c
    bmpformat.c
    converter.c
    gifformat.c
    icoformat.c
    info.c
    jpegformat.c
    metadata.c
    palette.c
    pngformat.c
    propertybag.c
    stream.c
    tiffformat.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    testlist.c)

add_executable(windowscodecs_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

set_module_type(windowscodecs_winetest win32cui)
add_importlibs(windowscodecs_winetest windowscodecs oleaut32 ole32 propsys user32 gdi32 shlwapi msvcrt kernel32)

if(MSVC)
    add_importlibs(windowscodecs_winetest ntdll)
endif()

add_pch(windowscodecs_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET windowscodecs_winetest)
