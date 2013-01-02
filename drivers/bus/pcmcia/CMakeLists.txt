
add_library(pcmcia SHARED
    fdo.c
    pcmcia.c
    pdo.c
    pcmcia.rc)

set_module_type(pcmcia kernelmodedriver)
add_pch(pcmcia pcmcia.h)
add_importlibs(pcmcia ntoskrnl hal)
add_cd_file(TARGET pcmcia DESTINATION reactos/system32/drivers NO_CAB FOR all)
