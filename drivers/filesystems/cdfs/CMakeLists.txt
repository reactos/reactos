
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    allocsup.c
    cachesup.c
    cddata.c
    cdinit.c
    cdprocssrc.c
    cleanup.c
    close.c
    create.c
    devctrl.c
    deviosup.c
    dirctrl.c
    dirsup.c
    fileinfo.c
    filobsup.c
    fsctrl.c
    fspdisp.c
    lockctrl.c
    namesup.c
    pathsup.c
    pnp.c
    prefxsup.c
    read.c
    resrcsup.c
    shutdown.c
    strucsup.c
    verfysup.c
    volinfo.c
    workque.c
    write.c)

add_library(cdfs MODULE ${SOURCE} cdfs.rc)
set_module_type(cdfs kernelmodedriver)
target_link_libraries(cdfs ${PSEH_LIB} memcmp)
add_importlibs(cdfs ntoskrnl hal)
set_property(TARGET cdfs PROPERTY C_STANDARD 90)

add_cd_file(TARGET cdfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(cdfs_reg.inf)
