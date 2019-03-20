list(APPEND SOURCE
    vfddbg.c
    vfddev.c
    vfddrv.c
    vfdfmt.c
    vfdimg.c
    vfdioctl.c
    vfdlink.c
    vfdmnt.c
    vfdpnp.c
    vfdrdwr.c)

include_directories(${REACTOS_SOURCE_DIR}/modules/rosapps/include/vfd)
add_library(vfddrv MODULE ${SOURCE} vfddrv.rc)
set_module_type(vfddrv kernelmodedriver)
add_importlibs(vfddrv ntoskrnl hal)
set_target_properties(vfddrv PROPERTIES OUTPUT_NAME "vfd")
add_cd_file(TARGET vfddrv DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(vfd_reg.inf)
