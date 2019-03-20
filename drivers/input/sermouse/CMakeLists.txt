
list(APPEND SOURCE
    createclose.c
    detect.c
    fdo.c
    internaldevctl.c
    misc.c
    readmouse.c
    sermouse.c
    sermouse.h)

add_library(sermouse MODULE ${SOURCE} sermouse.rc)
set_module_type(sermouse kernelmodedriver)
add_pch(sermouse sermouse.h SOURCE)
add_importlibs(sermouse ntoskrnl hal)
add_cd_file(TARGET sermouse DESTINATION reactos/system32/drivers FOR all)
