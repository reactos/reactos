
add_library(sermouse SHARED
    createclose.c
    detect.c
    fdo.c
    internaldevctl.c
    misc.c
    readmouse.c
    sermouse.c
    sermouse.rc)

set_module_type(sermouse kernelmodedriver)
add_pch(sermouse sermouse.h)
add_importlibs(sermouse ntoskrnl hal)
add_cd_file(TARGET sermouse DESTINATION reactos/system32/drivers FOR all)
