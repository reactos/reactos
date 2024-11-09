list(APPEND SOURCE
    blockdev.c
    btrfs.c
    cdfs.c
    ext.c
    fat.c
    fatx.c
    ffs.c
    fs_rec.c
    ntfs.c
    reiserfs.c
    udfs.c
    fs_rec.h)

add_library(fs_rec MODULE ${SOURCE} fs_rec.rc)
set_module_type(fs_rec kernelmodedriver)
add_importlibs(fs_rec ntoskrnl hal)
add_pch(fs_rec fs_rec.h SOURCE)
add_cd_file(TARGET fs_rec DESTINATION reactos/system32/drivers FOR all)
