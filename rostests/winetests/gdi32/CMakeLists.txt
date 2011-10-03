
set_rc_compiler()

add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

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
    testlist.c
    resource.rc)

add_executable(gdi32_winetest ${SOURCE})
allow_warnings(gdi32_winetest)
set_module_type(gdi32_winetest win32cui)
add_importlibs(gdi32_winetest gdi32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET gdi32_winetest DESTINATION reactos/bin FOR all)
