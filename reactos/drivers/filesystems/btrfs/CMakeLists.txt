
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers
                    inc)

list(APPEND SOURCE
    btrfs.c
    cache.c
    crc32c.c
    create.c
    dirctrl.c
    fastio.c
    fileinfo.c
    flushthread.c
    fsctl.c
    read.c
    reparse.c
    search.c
    security.c
    treefuncs.c
    write.c
    btrfs_drv.h)

add_library(btrfs SHARED ${SOURCE} btrfs.rc)

if(NOT MSVC)
    replace_compile_flags("-Werror" " ")
else()
    replace_compile_flags("/we\"4189\"" " ")
endif()

add_definitions(-D__KERNEL__)
set_module_type(btrfs kernelmodedriver)
target_link_libraries(btrfs ntoskrnl_vista)
add_importlibs(btrfs ntoskrnl hal)
add_pch(btrfs btrfs_drv.h SOURCE)
add_cd_file(TARGET btrfs DESTINATION reactos/system32/drivers NO_CAB FOR all)

