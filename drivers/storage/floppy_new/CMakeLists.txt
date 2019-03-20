list(APPEND SOURCE
    guid.c
    floppy.c)

add_library(floppy MODULE ${SOURCE} floppy.rc)
set_module_type(floppy kernelmodedriver)
add_importlibs(floppy classpnp hal ntoskrnl)
add_cd_file(TARGET floppy DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(floppy_reg.inf)
