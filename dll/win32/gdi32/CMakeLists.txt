
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-DLANGPACK)

include_directories(include)

list(APPEND SOURCE
    main/dllmain.c
    misc/heap.c
    misc/gdientry.c
    misc/hacks.c
    misc/historic.c
    misc/misc.c
    misc/stubs.c
    misc/stubsa.c
    misc/stubsw.c
    misc/wingl.c
    objects/arc.c
    objects/bitmap.c
    objects/brush.c
    objects/coord.c
    objects/dc.c
    objects/eng.c
    objects/enhmfile.c
    objects/font.c
    objects/icm.c
    objects/linedda.c
    objects/metafile.c
    objects/painting.c
    objects/printdrv.c
    objects/palette.c
    objects/pen.c
    objects/region.c
    objects/text.c
    objects/utils.c
    objects/path.c
    gdi32.rc)

add_library(gdi32 SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/gdi32_precomp.h.gch
    ${SOURCE})

set_module_type(gdi32 win32dll)

target_link_libraries(gdi32
    ${CMAKE_CURRENT_SOURCE_DIR}/gdi32.def
    win32ksys
    dxguid
    pseh)

add_importlibs(gdi32 user32 advapi32 kernel32 ntdll)
add_pch(gdi32 ${CMAKE_CURRENT_SOURCE_DIR}/include/precomp.h ${SOURCE})
add_livecd_target(gdi32 reactos/system32)
