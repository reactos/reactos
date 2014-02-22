
list(APPEND SOURCE
    enable.c
    font.c
    glyph.c
    bmfd.h)

add_library(bmfd SHARED ${SOURCE})
set_entrypoint(bmfd BmfdEnableDriver 12)
set_subsystem(bmfd native)
set_image_base(bmfd 0x00010000)
target_link_libraries(bmfd libcntpr)
add_importlibs(bmfd win32k)
add_pch(bmfd bmfd.h SOURCE)
add_dependencies(bmfd psdk bugcodes)
