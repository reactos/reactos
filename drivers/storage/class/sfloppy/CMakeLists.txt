list(APPEND SOURCE
    floppy.c
    guid.c)

add_library(sfloppy MODULE ${SOURCE} sfloppy.rc)
set_module_type(sfloppy kernelmodedriver)
add_importlibs(sfloppy classpnp hal ntoskrnl)
add_cd_file(TARGET sfloppy DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(sfloppy_reg.inf)
