
list(APPEND SOURCE
    create.c
    finfo.c
    fsctrl.c
    msfs.c
    rw.c
    msfs.rc)

add_library(msfs SHARED ${SOURCE})

set_module_type(msfs kernelmodedriver)
add_importlibs(msfs ntoskrnl hal)

add_pch(msfs msfs.h)

add_cd_file(TARGET msfs DESTINATION reactos/system32/drivers FOR all)
