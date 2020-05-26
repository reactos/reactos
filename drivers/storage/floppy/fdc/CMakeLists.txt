
list(APPEND SOURCE
    fdc.c
    fdo.c
    misc.c
    pdo.c
    fdc.h)

add_library(fdc MODULE ${SOURCE} fdc.rc)
set_module_type(fdc kernelmodedriver)
add_importlibs(fdc hal ntoskrnl)
add_pch(fdc fdc.h SOURCE)
add_cd_file(TARGET fdc DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(fdc fdc.inf)
