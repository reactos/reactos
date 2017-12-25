
add_definitions(
    -D__WINESRC__
    -D_USE_MATH_DEFINES)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(gdiplus.dll gdiplus.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    brush.c
    customlinecap.c
    font.c
    gdiplus.c
    graphics.c
    graphicspath.c
    image.c
    imageattributes.c
    matrix.c
    metafile.c
    pathiterator.c
    pen.c
    region.c
    stringformat.c
    gdiplus_private.h)

add_library(gdiplus SHARED
    ${SOURCE}
    guid.c
    gdiplus.rc
    ${CMAKE_CURRENT_BINARY_DIR}/gdiplus_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/gdiplus.def)

set_module_type(gdiplus win32dll)
target_link_libraries(gdiplus wine)
add_delay_importlibs(gdiplus windowscodecs)
add_importlibs(gdiplus msvcrt shlwapi oleaut32 ole32 user32 gdi32 kernel32 ntdll)
add_pch(gdiplus gdiplus_private.h SOURCE NO_UNITY_BUILD)
add_cd_file(TARGET gdiplus DESTINATION reactos/system32 FOR all)
