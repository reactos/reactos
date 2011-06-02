
list(APPEND SOURCE
    initvga.c
    vgamp.c
    vgamp.rc)

add_library(vgamp SHARED ${SOURCE})

set_module_type(vgamp kernelmodedriver)
add_importlibs(vgamp videoprt)

add_pch(vgamp vgamp.h)
add_cd_file(TARGET vgamp DESTINATION reactos/system32/drivers FOR all)
