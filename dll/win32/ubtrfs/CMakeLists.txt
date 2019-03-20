
spec2def(ubtrfs.dll ubtrfs.spec)

list(APPEND SOURCE
    ubtrfs.c
    ubtrfs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ubtrfs.def)

add_library(ubtrfs MODULE ${SOURCE})
set_module_type(ubtrfs nativedll)
target_link_libraries(ubtrfs btrfslib)
add_importlibs(ubtrfs ntdll)
add_cd_file(TARGET ubtrfs DESTINATION reactos/system32 FOR all)
