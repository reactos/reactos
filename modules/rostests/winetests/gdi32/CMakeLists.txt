
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
    pen.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(gdi32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    resource.rc)

if(MSVC AND ARCH STREQUAL "amd64")
    # warning C4334: '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    target_compile_options(gdi32_winetest PRIVATE /wd4334)
endif()

set_module_type(gdi32_winetest win32cui)
add_importlibs(gdi32_winetest gdi32 gdi32_vista user32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(gdi32_winetest ntdll)
endif()

add_pch(gdi32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET gdi32_winetest)
