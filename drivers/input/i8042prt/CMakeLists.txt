
add_library(i8042prt SHARED
    createclose.c
    i8042prt.c
    keyboard.c
    misc.c
    mouse.c
    pnp.c
    ps2pp.c
    readwrite.c
    registry.c
    setup.c
    i8042prt.rc)

set_module_type(i8042prt kernelmodedriver)
add_importlibs(i8042prt ntoskrnl hal)

add_minicd_target(i8042prt reactos i8042prt.sys)
add_livecd_target(i8042prt reactos/system32/drivers)