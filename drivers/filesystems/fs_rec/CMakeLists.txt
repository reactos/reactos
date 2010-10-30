
add_library(fs_rec SHARED
    blockdev.c
    cdfs.c
    ext2.c
    fat.c
    fs_rec.c
    ntfs.c
    udfs.c
    fs_rec.rc)

set_target_properties(fs_rec PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(fs_rec ntoskrnl hal)
add_dependencies(fs_rec psdk bugcodes)
add_cab_target(fs_rec 2)