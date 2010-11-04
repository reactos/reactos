
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

set_module_type(serial kernelmodedriver)
add_importlibs(serial ntoskrnl hal)

add_cab_target(serial 2)
