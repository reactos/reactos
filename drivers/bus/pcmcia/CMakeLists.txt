
list(APPEND SOURCE
    fdo.c
    pcmcia.c
    pdo.c
    pcmcia.h)

add_library(pcmcia MODULE ${SOURCE} pcmcia.rc)
set_module_type(pcmcia kernelmodedriver)
add_pch(pcmcia pcmcia.h SOURCE)
add_importlibs(pcmcia ntoskrnl hal)
add_cd_file(TARGET pcmcia DESTINATION reactos/system32/drivers NO_CAB FOR all)
