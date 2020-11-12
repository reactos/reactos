list(APPEND SOURCE
    floppy.c
    guid.c)

add_library(floppy MODULE ${SOURCE} sfloppy.rc)
set_module_type(floppy kernelmodedriver)
add_importlibs(floppy classpnp hal ntoskrnl)
add_cd_file(TARGET floppy DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(sfloppy_reg.inf)
