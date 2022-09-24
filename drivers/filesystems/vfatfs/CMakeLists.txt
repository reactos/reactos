
list(APPEND SOURCE
    blockdev.c
    cleanup.c
    close.c
    create.c
    dir.c
    direntry.c
    dirwr.c
    ea.c
    fat.c
    fastio.c
    fcb.c
    finfo.c
    flush.c
    fsctl.c
    iface.c
    kdbg.c
    misc.c
    pnp.c
    rw.c
    shutdown.c
    string.c
    volume.c
    vfat.h)

if(KDBG)
    add_definitions(-DKDBG)
endif()
add_library(vfatfs MODULE ${SOURCE} vfatfs.rc)
set_module_type(vfatfs kernelmodedriver)
target_link_libraries(vfatfs ${PSEH_LIB})
add_importlibs(vfatfs ntoskrnl hal)
add_pch(vfatfs vfat.h SOURCE)
if(SARCH STREQUAL "xbox")
    add_cd_file(TARGET vfatfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
    add_registry_inf(vfatfs_reg.inf)
endif()
