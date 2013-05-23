
add_definitions(-D__ROS_LONG64__)

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
target_link_libraries(windowscodecs_winetest wine)
set_module_type(windowscodecs_winetest win32cui)
add_importlibs(windowscodecs_winetest windowscodecs oleaut32 ole32 user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET windowscodecs_winetest DESTINATION reactos/bin FOR all)
