
list(APPEND SOURCE
    edid.c
    vbemp.c
    vbemp.h)

add_library(vgapnp MODULE ${SOURCE} vbemp.rc)
set_module_type(vgapnp kernelmodedriver)
add_importlibs(vgapnp videoprt)
add_pch(vgapnp vbemp.h SOURCE)
target_link_libraries(vgapnp libcntpr)
add_cd_file(TARGET vgapnp DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(vbemp_reg.inf)
