
list(APPEND SOURCE
    ureiserfs.c
    ureiserfs.rc)

add_library(ureiserfs MODULE ${SOURCE})
spec2def(ureiserfs ureiserfs.dll ureiserfs.spec)

set_module_type(ureiserfs nativedll)
target_link_libraries(ureiserfs reiserfslib)
add_importlibs(ureiserfs ntdll)
add_cd_file(TARGET ureiserfs DESTINATION reactos/system32 FOR all)
