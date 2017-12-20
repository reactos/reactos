
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    brush.c
    customlinecap.c
    font.c
    graphics.c
    graphicspath.c
    image.c
    matrix.c
    metafile.c
    pathiterator.c
    pen.c
    region.c
    stringformat.c
    precomp.h)

add_executable(gdiplus_winetest
    ${SOURCE}
    guid.c
    testlist.c)

set_module_type(gdiplus_winetest win32cui)
add_importlibs(gdiplus_winetest gdiplus user32 gdi32 ole32 msvcrt kernel32)
add_pch(gdiplus_winetest precomp.h SOURCE)
add_rostests_file(TARGET gdiplus_winetest)
