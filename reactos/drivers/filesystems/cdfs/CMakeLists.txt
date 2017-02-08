
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

list(APPEND SOURCE
    cdfs.c
    cleanup.c
    close.c
    common.c
    create.c
    devctrl.c
    dirctl.c
    dispatch.c
    fastio.c
    fcb.c
    finfo.c
    fsctl.c
    misc.c
    rw.c
    volinfo.c
    cdfs.h)

add_library(cdfs SHARED ${SOURCE} cdfs.rc)
set_module_type(cdfs kernelmodedriver)
target_link_libraries(cdfs ${PSEH_LIB})
add_importlibs(cdfs ntoskrnl hal)
add_pch(cdfs cdfs.h SOURCE)
add_cd_file(TARGET cdfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(cdfs_reg.inf)
