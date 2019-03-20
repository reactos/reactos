
list(APPEND SOURCE
    detect.c
    fdo.c
    misc.c
    pdo.c
    serenum.c
    serenum.h)

add_library(serenum MODULE
    ${SOURCE}
    guid.c
    serenum.rc)

add_pch(serenum serenum.h SOURCE)
set_module_type(serenum kernelmodedriver)
add_importlibs(serenum ntoskrnl hal)
add_cd_file(TARGET serenum DESTINATION reactos/system32/drivers FOR all)
