
include_directories(
    .
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/fullfat)

list(APPEND SOURCE
    cleanup.c
    close.c
    create.c
    device.c
    dir.c
    ea.c
    fastfat.c
    fastio.c
    fat.c
    fcb.c
    finfo.c
    flush.c
    fsctl.c
    fullfat.c
    lock.c
    rw.c
    shutdown.c
    volume.c
    fastfat.rc)

add_library(fastfat_new SHARED ${SOURCE})

set_module_type(fastfat_new kernelmodedriver)

target_link_libraries(fastfat_new
    ${PSEH_LIB}
    fullfat)

add_importlibs(fastfat_new ntoskrnl hal)

add_pch(fastfat_new fastfat.h)
