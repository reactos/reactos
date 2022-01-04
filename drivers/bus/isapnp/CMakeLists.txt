
if(ISAPNP_ENABLE)
    list(APPEND SOURCE
        fdo.c
        hardware.c
        interface.c
        isapnp.c
        isapnp.h
        pdo.c)
else()
    list(APPEND SOURCE
        interface.c
        isapnp.h
        stub.c)
endif()

add_library(isapnp MODULE ${SOURCE} isapnp.rc)
set_module_type(isapnp kernelmodedriver)
add_importlibs(isapnp ntoskrnl hal)
add_pch(isapnp isapnp.h SOURCE)
add_cd_file(TARGET isapnp DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(isapnp_reg.inf)
