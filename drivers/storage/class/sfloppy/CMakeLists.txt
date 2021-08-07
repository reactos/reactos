
list(APPEND SOURCE
    guid.c
    floppy.c)

add_library(sfloppy MODULE ${SOURCE} floppy.rc)
set_module_type(sfloppy kernelmodedriver)
add_importlibs(sfloppy classpnp hal ntoskrnl)
add_cd_file(TARGET sfloppy DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(sfloppy sfloppy.inf)
add_registry_inf(sfloppy_reg.inf)
