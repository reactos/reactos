
list(APPEND SOURCE
    ubtrfs.c
    ubtrfs.rc)

add_library(ubtrfs MODULE ${SOURCE})
spec2def(ubtrfs ubtrfs.dll ubtrfs.spec)

set_module_type(ubtrfs nativedll)
target_link_libraries(ubtrfs btrfslib)
add_importlibs(ubtrfs ntdll)
add_cd_file(TARGET ubtrfs DESTINATION reactos/system32 FOR all)
