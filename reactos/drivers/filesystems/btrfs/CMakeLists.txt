
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers
                    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
                    inc)

list(APPEND SOURCE
    balance.c
    btrfs.c
    cache.c
    calcthread.c
    compress.c
    crc32c.c
    create.c
    devctrl.c
    dirctrl.c
    extent-tree.c
    fastio.c
    fileinfo.c
    flushthread.c
    free-space.c
    fsctl.c
    galois.c
    pnp.c
    read.c
    registry.c
    reparse.c
    search.c
    security.c
    treefuncs.c
    worker-thread.c
    write.c
    btrfs_drv.h)

add_library(btrfs SHARED ${SOURCE} btrfs.rc)

add_definitions(-D__KERNEL__)
set_module_type(btrfs kernelmodedriver)
target_link_libraries(btrfs rtlver ntoskrnl_vista zlib_solo wdmguid ${PSEH_LIB})
add_importlibs(btrfs ntoskrnl hal)
add_pch(btrfs btrfs_drv.h SOURCE)
add_cd_file(TARGET btrfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
