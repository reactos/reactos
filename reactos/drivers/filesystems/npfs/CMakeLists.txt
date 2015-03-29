
list(APPEND SOURCE
    cleanup.c
    close.c
    create.c
    datasup.c
    fileinfo.c
    fileobsup.c
    flushbuf.c
    fsctrl.c
    main.c
    prefxsup.c
    read.c
    readsup.c
    secursup.c
    seinfo.c
    statesup.c
    strucsup.c
    volinfo.c
    waitsup.c
    write.c
    writesup.c
    npfs.h)

add_library(npfs SHARED ${SOURCE})
set_module_type(npfs kernelmodedriver)
target_link_libraries(npfs ${PSEH_LIB})
add_importlibs(npfs ntoskrnl hal)
add_pch(npfs npfs.h SOURCE)
add_cd_file(TARGET npfs DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(npfs_reg.inf)
