
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers
                    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
                    inc)

list(APPEND SOURCE
    balance.c
    boot.c
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
    fsrtl.c
    galois.c
    guid.c
    pnp.c
    read.c
    registry.c
    reparse.c
    scrub.c
    search.c
    security.c
    send.c
    treefuncs.c
    volume.c
    worker-thread.c
    write.c
    zstd/entropy_common.c
    zstd/fse_compress.c
    zstd/hist.c
    zstd/huf_decompress.c
    zstd/zstd_common.c
    zstd/zstd_decompress.c
    zstd/zstd_fast.c
    zstd/zstd_ldm.c
    zstd/error_private.c
    zstd/fse_decompress.c
    zstd/huf_compress.c
    zstd/xxhash.c
    zstd/zstd_compress.c
    zstd/zstd_double_fast.c
    zstd/zstd_lazy.c
    zstd/zstd_opt.c
    btrfs_drv.h)

add_library(btrfs MODULE ${SOURCE} btrfs.rc)

add_definitions(-D__KERNEL__)
set_module_type(btrfs kernelmodedriver)
target_link_libraries(btrfs rtlver zlib_solo chkstk wdmguid ${PSEH_LIB})
add_importlibs(btrfs ntoskrnl hal)
add_cd_file(TARGET btrfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
