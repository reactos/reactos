
list(APPEND SOURCE
    createclose.c
    hwhacks.c
    i8042prt.c
    keyboard.c
    misc.c
    mouse.c
    pnp.c
    ps2pp.c
    readwrite.c
    registry.c
    i8042prt.h)

add_library(i8042prt SHARED
    ${SOURCE}
    guid.c
    i8042prt.rc)

set_module_type(i8042prt kernelmodedriver)
add_importlibs(i8042prt ntoskrnl hal)
add_pch(i8042prt i8042prt.h SOURCE)
add_cd_file(TARGET i8042prt DESTINATION reactos/system32/drivers NO_CAB FOR all)
