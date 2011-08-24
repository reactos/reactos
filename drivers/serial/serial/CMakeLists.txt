
add_library(serial SHARED
    circularbuffer.c
    cleanup.c
    close.c
    create.c
    devctrl.c
    info.c
    legacy.c
    misc.c
    pnp.c
    power.c
    rw.c
    serial.c
    serial.rc)

add_pch(serial serial.h)
set_module_type(serial kernelmodedriver)
add_importlibs(serial ntoskrnl hal)
add_cd_file(TARGET serial DESTINATION reactos/system32/drivers FOR all)
