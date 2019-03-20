
list(APPEND SOURCE
    createclose.c
    dispatch.c
    green.c
    keyboard.c
    misc.c
    pnp.c
    power.c
    screen.c
    green.h)

add_library(green MODULE ${SOURCE} green.rc)
add_pch(green green.h SOURCE)
set_module_type(green kernelmodedriver)
add_importlibs(green ntoskrnl hal)
add_cd_file(TARGET green DESTINATION reactos/system32/drivers NO_CAB FOR all)
