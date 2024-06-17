
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers
                    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
                    inc)

list(APPEND SOURCE
    balance.c
    blake2b-ref.c
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
    sha256.c
    send.c
    treefuncs.c
    volume.c
    worker-thread.c
    write.c
    xxhash.c
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
    zstd/zstd_compress.c
    zstd/zstd_double_fast.c
    zstd/zstd_lazy.c
    zstd/zstd_opt.c
    btrfs_drv.h)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE crc32c-x86.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE crc32c-amd64.S)
endif()

add_asm_files(btrfs_asm ${ASM_SOURCE})

add_library(btrfs MODULE ${SOURCE} ${btrfs_asm} btrfs.rc)

add_definitions(-D__KERNEL__)
set_module_type(btrfs kernelmodedriver)
target_link_libraries(btrfs rtlver zlib_solo chkstk wdmguid ${PSEH_LIB})
add_importlibs(btrfs ntoskrnl hal)
add_cd_file(TARGET btrfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(btrfs btrfs.inf)
