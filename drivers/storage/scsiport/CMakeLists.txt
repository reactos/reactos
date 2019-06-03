
list(APPEND SOURCE
    scsiport.c
    stubs.c
    precomp.h)

add_library(scsiport MODULE
    ${SOURCE}
    scsiport.rc)
spec2def(scsiport scsiport.sys scsiport.spec ADD_IMPORTLIB)

add_pch(scsiport precomp.h SOURCE)
set_module_type(scsiport kernelmodedriver)
add_importlibs(scsiport ntoskrnl hal)
add_cd_file(TARGET scsiport DESTINATION reactos/system32/drivers NO_CAB FOR all)
