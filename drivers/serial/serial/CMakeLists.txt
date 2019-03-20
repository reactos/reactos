
list(APPEND SOURCE
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
    serial.h)

add_library(serial MODULE
    ${SOURCE}
    guid.c
    serial.rc)

set_module_type(serial kernelmodedriver)
add_importlibs(serial ntoskrnl hal)
add_pch(serial serial.h SOURCE)
add_cd_file(TARGET serial DESTINATION reactos/system32/drivers FOR all)
