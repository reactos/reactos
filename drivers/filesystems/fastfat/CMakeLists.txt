
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
    misc.c
    pnp.c
    rw.c
    shutdown.c
    string.c
    volume.c
    vfatfs.rc)

add_library(fastfat SHARED ${SOURCE})

set_module_type(fastfat kernelmodedriver)
add_importlibs(fastfat ntoskrnl hal)

add_pch(fastfat vfat.h)

add_cd_file(TARGET fastfat DESTINATION reactos/system32/drivers NO_CAB FOR all)
