
list(APPEND SOURCE
    modeset.c
    vgadata.c
    vga.c
    vbemodes.c
    vbe.c
    vga.h)

add_library(vga MODULE
    ${SOURCE}
    vga.rc)

target_link_libraries(vga libcntpr)
set_module_type(vga kernelmodedriver)
add_importlibs(vga videoprt)
add_pch(vga vga.h SOURCE)

# pc98vid takes place of vga.sys on PC-98 platform
if(NOT SARCH STREQUAL "pc98")
    add_cd_file(TARGET vga DESTINATION reactos/system32/drivers FOR all)
endif()
