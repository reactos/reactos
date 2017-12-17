
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    bitmap.c
    brush.c
    clipping.c
    dc.c
    dib.c
    font.c
    gdiobj.c
    generated.c
    icm.c
    mapping.c
    metafile.c
    palette.c
    path.c
    pen.c
    precomp.h)

add_executable(gdi32_winetest
    ${SOURCE}
    testlist.c
    resource.rc)

set_module_type(gdi32_winetest win32cui)
add_importlibs(gdi32_winetest gdi32 user32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(gdi32_winetest ntdll)
endif()

add_pch(gdi32_winetest precomp.h SOURCE)
add_rostests_file(TARGET gdi32_winetest)
