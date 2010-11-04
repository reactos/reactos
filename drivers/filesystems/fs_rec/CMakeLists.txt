
add_library(fs_rec SHARED
    blockdev.c
    cdfs.c
    ext2.c
    fat.c
    fs_rec.c
    ntfs.c
    udfs.c
    fs_rec.rc)

set_module_type(fs_rec kernelmodedriver)
add_importlibs(fs_rec ntoskrnl hal)

add_cab_target(fs_rec 2)
