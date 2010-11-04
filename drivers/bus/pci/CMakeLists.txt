
add_library(pci SHARED
    fdo.c
    pci.c
    pdo.c
    pci.rc)

set_module_type(pci kernelmodedriver)
add_importlibs(pci ntoskrnl hal)

add_minicd_target(pci reactos pci.sys)
add_livecd_target(pci reactos/system32/drivers)
