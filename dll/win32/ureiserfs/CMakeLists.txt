
spec2def(ureiserfs.dll ureiserfs.spec)

list(APPEND SOURCE
    ureiserfs.c
    ureiserfs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ureiserfs.def)

add_library(ureiserfs MODULE ${SOURCE})
set_module_type(ureiserfs nativedll)
target_link_libraries(ureiserfs reiserfslib)
add_importlibs(ureiserfs ntdll)
add_cd_file(TARGET ureiserfs DESTINATION reactos/system32 FOR all)
