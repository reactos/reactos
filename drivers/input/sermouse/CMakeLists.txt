
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
add_importlibs(sermouse ntoskrnl hal)

add_cab_target(sermouse 2)
