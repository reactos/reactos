
list(APPEND SOURCE
    detect.c
    fdo.c
    misc.c
    pdo.c
    serenum.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(serenum MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    serenum.rc)

add_pch(serenum serenum.h "${PCH_SKIP_SOURCE}")
set_module_type(serenum kernelmodedriver)
add_importlibs(serenum ntoskrnl hal)
add_cd_file(TARGET serenum DESTINATION reactos/system32/drivers FOR all)
