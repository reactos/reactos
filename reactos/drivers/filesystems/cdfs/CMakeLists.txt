
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

list(APPEND SOURCE
    cdfs.c
    cleanup.c
    close.c
    common.c
    create.c
    devctrl.c
    dirctl.c
    fcb.c
    finfo.c
    fsctl.c
    misc.c
    rw.c
    volinfo.c
    cdfs.rc)

add_library(cdfs SHARED ${CMAKE_CURRENT_BINARY_DIR}/cdfs_cdfs.h.gch ${SOURCE})

set_module_type(cdfs kernelmodedriver)
add_importlibs(cdfs ntoskrnl hal)

add_pch(cdfs ${CMAKE_CURRENT_SOURCE_DIR}/cdfs.h ${SOURCE})

add_cd_file(TARGET cdfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
