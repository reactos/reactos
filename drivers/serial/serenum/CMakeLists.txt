
add_library(serenum SHARED
    detect.c
    fdo.c
    misc.c
    pdo.c
    serenum.c
    serenum.rc)

add_pch(serenum serenum.h)
set_module_type(serenum kernelmodedriver)
add_importlibs(serenum ntoskrnl hal)
add_cd_file(TARGET serenum DESTINATION reactos/system32/drivers FOR all)
