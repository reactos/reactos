
add_library(isapnp SHARED
    isapnp.c
    pdo.c
    fdo.c
    hardware.c
    isapnp.rc)

set_module_type(isapnp kernelmodedriver)
add_importlibs(isapnp ntoskrnl hal)
add_pch(isapnp isapnp.h)
add_cd_file(TARGET isapnp DESTINATION reactos/system32/drivers NO_CAB FOR all)
