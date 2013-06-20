
add_library(fdc SHARED
    fdc.c
    fdo.c
    pdo.c
    fdc.rc)

set_module_type(fdc kernelmodedriver)
add_importlibs(fdc hal ntoskrnl)

add_cd_file(TARGET fdc DESTINATION reactos/system32/drivers NO_CAB FOR all)
