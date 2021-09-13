
add_subdirectory(wine)

add_definitions(-DLANGPACK)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-DLANGPACK)
spec2def(gdi32.dll gdi32.spec ADD_IMPORTLIB)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE objects/efloat.c)
endif()

list(APPEND SOURCE
    main/dllmain.c
    misc/heap.c
    misc/gdientry.c
    misc/historic.c
    misc/misc.c
    misc/wingl.c
    objects/arc.c
    objects/bitmap.c
    objects/brush.c
    objects/clientobj.c
    objects/colorspace.c
    objects/coord.c
    objects/dc.c
    objects/eng.c
    objects/enhmfile.c
    objects/font.c
    objects/gdiobj.c
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
    include/precomp.h)

add_library(gdi32 MODULE
    ${SOURCE}
    gdi32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/gdi32.def)

set_module_type(gdi32
    win32dll
    ENTRYPOINT DllMain 12
    UNICODE)

target_link_libraries(gdi32
    winegdi
    wine
    win32ksys
    dxguid
    atan2
    ${PSEH_LIB})

if(MSVC AND (ARCH STREQUAL "i386"))
    target_sources(gdi32 PRIVATE $<TARGET_OBJECTS:ftol2_sse>)
endif()

add_importlibs(gdi32 user32 advapi32 kernel32 ntdll)
add_pch(gdi32 include/precomp.h SOURCE)
add_dependencies(gdi32 psdk)
add_cd_file(TARGET gdi32 DESTINATION reactos/system32 FOR all)
