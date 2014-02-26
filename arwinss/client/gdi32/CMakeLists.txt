

add_definitions(-DLANGPACK)
add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/arwinss/include
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(gdi32.dll gdi32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    atan2.c
    bidi.c
    bitblt.c
    bitmap.c
    brush.c
    clipping.c
    dc.c
    dib.c
    driver.c
    enhmetafile.c
    enhmeta2.c
    font.c
    freetype.c
    gdiobj.c
    icm.c
    mapping.c
    metafile.c
    opengl.c
    painting.c
    palette.c
    path.c
    pen.c
    printdrv.c
    region.c
    regglue.c
    historic.c
    gdi32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/gdi32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/gdi32.def)

add_library(gdi32 SHARED ${SOURCE})

set_module_type(gdi32 win32dll UNICODE)

target_link_libraries(gdi32
    win32ksys
    dxguid
    wine
    ${PSEH_LIB})

add_importlibs(gdi32 user32 freetype usp10 msvcrt kernel32 ntdll)
#add_pch(gdi32 include/precomp.h)
add_cd_file(TARGET gdi32 DESTINATION reactos/system32 FOR all)
