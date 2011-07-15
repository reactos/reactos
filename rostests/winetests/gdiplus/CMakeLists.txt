
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    brush.c
    customlinecap.c
    font.c
    graphics.c
    graphicspath.c
    image.c
    matrix.c
    pathiterator.c
    pen.c
    region.c
    stringformat.c
    testlist.c)

add_executable(gdiplus_winetest ${SOURCE})
target_link_libraries(gdiplus_winetest wine)
set_module_type(gdiplus_winetest win32cui)
add_importlibs(gdiplus_winetest gdiplus user32 gdi32 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET gdiplus_winetest DESTINATION reactos/bin FOR all)
