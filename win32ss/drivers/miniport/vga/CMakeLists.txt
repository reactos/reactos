
list(APPEND SOURCE
    initvga.c
    vgamp.c
    vgamp.h)

add_library(vga MODULE ${SOURCE} vgamp.rc)
set_module_type(vga kernelmodedriver)
add_importlibs(vga videoprt)
add_pch(vga vgamp.h SOURCE)
add_cd_file(TARGET vga DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(vga_reg.inf)
