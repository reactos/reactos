
include_directories(
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs)

list(APPEND SOURCE
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs/blake2b-ref.c
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs/crc32c.c
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs/sha256.c
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs/xxhash.c
    btrfslib.c)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE ${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs/crc32c-x86.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE ${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs/crc32c-amd64.S)
endif()

add_asm_files(btrfs_asm ${ASM_SOURCE})

add_library(btrfslib ${SOURCE} ${btrfs_asm})
target_compile_definitions(btrfslib PRIVATE _USRDLL)
add_dependencies(btrfslib psdk)
