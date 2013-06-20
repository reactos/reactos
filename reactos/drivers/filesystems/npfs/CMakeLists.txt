
list(APPEND SOURCE
    create.c
    dirctl.c
    finfo.c
    fsctrl.c
    npfs.c
    rw.c
    volume.c
    npfs.rc)

add_library(npfs SHARED ${SOURCE})

target_link_libraries(npfs ${PSEH_LIB})

set_module_type(npfs kernelmodedriver)
add_importlibs(npfs ntoskrnl hal)
add_pch(npfs npfs.h)
add_cd_file(TARGET npfs DESTINATION reactos/system32/drivers FOR all)
