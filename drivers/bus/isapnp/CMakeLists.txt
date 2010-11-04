
add_library(isapnp SHARED
    isapnp.c
    pdo.c
    fdo.c
    hardware.c
    isapnp.rc)

set_module_type(isapnp kernelmodedriver)
add_importlibs(isapnp ntoskrnl hal)

add_minicd_target(isapnp reactos isapnp.sys)
add_livecd_target(isapnp reactos/system32/drivers)
