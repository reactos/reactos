
list(APPEND SOURCE
    modeset.c
    vgadata.c
    vga.c
    vbemodes.c
    vbe.c
    vga.h)

add_library(vga SHARED
    ${SOURCE}
    vga.rc)

target_link_libraries(vga libcntpr)
set_module_type(vga kernelmodedriver)
add_importlibs(vga videoprt)
add_pch(vga vga.h SOURCE)
add_cd_file(TARGET vga DESTINATION reactos/system32/drivers FOR all)
