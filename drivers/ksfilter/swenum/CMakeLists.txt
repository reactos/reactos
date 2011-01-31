
list(APPEND SOURCE
    swenum.c)

add_library(swenum SHARED ${SOURCE})

set_module_type(swenum kernelmodedriver)
add_importlibs(swenum ks ntoskrnl hal)

add_minicd_target(swenum reactos)
add_livecd_target(swenum reactos/system32/drivers)
