
list(APPEND SOURCE
    create.c
    finfo.c
    fsctrl.c
    msfs.c
    msfssup.c
    rw.c
    msfs.h)

add_library(msfs SHARED ${SOURCE} msfs.rc)
set_module_type(msfs kernelmodedriver)
add_importlibs(msfs ntoskrnl hal)
add_pch(msfs msfs.h SOURCE)
add_cd_file(TARGET msfs DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(msfs_reg.inf)
