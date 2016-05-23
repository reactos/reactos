
# include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

list(APPEND SOURCE
    allocsup.c
    cachesup.c
    cddata.c
    cdinit.c
    cleanup.c
    close.c
    create.c
    devctrl.c
    deviosup.c
    dirctrl.c
    dirsup.c
#   fieldoff.c #useless
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
    strucsup.c
    verfysup.c
    volinfo.c
    workque.c
    cdprocs.h)

add_library(cdfs SHARED ${SOURCE} cdfs.rc)
set_module_type(cdfs kernelmodedriver)
target_link_libraries(cdfs memcmp ${PSEH_LIB})
add_importlibs(cdfs ntoskrnl hal)
add_pch(cdfs cdprocs.h SOURCE)
add_cd_file(TARGET cdfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(cdfs_reg.inf)
