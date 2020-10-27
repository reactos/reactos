
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
    stringformat.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    testlist.c)

add_executable(gdiplus_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    resource.rc)

set_module_type(gdiplus_winetest win32cui)
add_importlibs(gdiplus_winetest gdiplus user32 gdi32 ole32 msvcrt kernel32)
add_pch(gdiplus_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET gdiplus_winetest)

if(MSVC)
    # error C4133: 'function': incompatible types - from 'ImageFlags *' to 'UINT *'
    remove_target_compile_option(gdiplus_winetest "/we4133")
endif()
