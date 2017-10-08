
list(APPEND SOURCE
    enable.c
    font.c
    glyph.c
    bmfd.h)

add_library(bmfd SHARED ${SOURCE})
set_module_type(bmfd kerneldll ENTRYPOINT BmfdEnableDriver 12)
target_link_libraries(bmfd libcntpr)
add_importlibs(bmfd win32k)
add_pch(bmfd bmfd.h SOURCE)
add_dependencies(bmfd psdk)
