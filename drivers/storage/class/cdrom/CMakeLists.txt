
include_directories(${REACTOS_SOURCE_DIR}/drivers/storage/class)

list(APPEND SOURCE
    cdrom.c
    findscsi.c
    precomp.h)

add_library(cdrom MODULE ${SOURCE} cdrom.rc)
set_module_type(cdrom kernelmodedriver)
add_importlibs(cdrom class2 scsiport ntoskrnl hal)
add_pch(cdrom precomp.h SOURCE)
add_cd_file(TARGET cdrom DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(cdrom_reg.inf)
add_driver_inf(cdrom cdrom.inf)
