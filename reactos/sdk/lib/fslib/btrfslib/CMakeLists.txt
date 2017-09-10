
include_directories(
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/btrfs)

add_library(btrfslib btrfslib.c)
add_dependencies(btrfslib psdk)
