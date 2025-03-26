
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers
                    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
                    inc)

set(ZSTD_SRC_FILES
    zstd/entropy_common.c
    zstd/error_private.c
    zstd/fse_compress.c
    zstd/fse_decompress.c
    zstd/hist.c
    zstd/huf_compress.c
    zstd/huf_decompress.c
    zstd/zstd_common.c
    zstd/zstd_compress.c
    zstd/zstd_compress_literals.c
    zstd/zstd_compress_sequences.c
    zstd/zstd_compress_superblock.c
    zstd/zstd_ddict.c
    zstd/zstd_decompress.c
    zstd/zstd_decompress_block.c
    zstd/zstd_double_fast.c
    zstd/zstd_fast.c
    zstd/zstd_lazy.c
    zstd/zstd_ldm.c
    zstd/zstd_opt.c)

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
    ${ZSTD_SRC_FILES}
    btrfs_drv.h)

if((ARCH STREQUAL "i386") OR (ARCH STREQUAL "amd64"))
    list(APPEND ASM_SOURCE crc32c.S xor.S)
    add_asm_files(btrfs_asm ${ASM_SOURCE})
endif()

add_library(btrfs MODULE ${SOURCE} ${btrfs_asm} btrfs.rc)

if(MSVC)
    # Disable warning C4267: 'function': conversion from 'size_t' to 'uint16_t', possible loss of data
    target_compile_options(btrfs PRIVATE /wd4267)
endif()

add_definitions(-D__KERNEL__)
set_module_type(btrfs kernelmodedriver)
target_link_libraries(btrfs rtlver zlib_solo chkstk wdmguid ${PSEH_LIB})
add_importlibs(btrfs ntoskrnl hal)

add_cd_file(TARGET btrfs DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(btrfs btrfs.inf)
add_registry_inf(btrfs_reg.inf)
