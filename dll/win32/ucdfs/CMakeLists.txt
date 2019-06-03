
list(APPEND SOURCE
    ucdfs.c
    ucdfs.rc)

add_library(ucdfs MODULE ${SOURCE})
spec2def(ucdfs ucdfs.dll ucdfs.spec)

set_module_type(ucdfs nativedll)
target_link_libraries(ucdfs cdfslib)
add_importlibs(ucdfs ntdll)
add_cd_file(TARGET ucdfs DESTINATION reactos/system32 FOR all)
