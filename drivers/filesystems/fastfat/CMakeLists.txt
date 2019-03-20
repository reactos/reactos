
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

add_library(fastfat MODULE ${SOURCE} vfatfs.rc)
set_module_type(fastfat kernelmodedriver)
target_link_libraries(fastfat ${PSEH_LIB})
add_importlibs(fastfat ntoskrnl hal)
add_pch(fastfat vfat.h SOURCE)
add_cd_file(TARGET fastfat DESTINATION reactos/system32/drivers NO_CAB FOR all)
