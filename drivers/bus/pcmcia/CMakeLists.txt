
add_library(pcmcia SHARED
    fdo.c
    pcmcia.c
    pdo.c
    pcmcia.rc)

set_module_type(pcmcia kernelmodedriver)
add_importlibs(pcmcia ntoskrnl hal)

add_minicd_target(pcmcia reactos pcmcia.sys)
add_livecd_target(pcmcia reactos/system32/drivers)
