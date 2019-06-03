
list(APPEND SOURCE
    untfs.c
    untfs.rc)

add_library(untfs MODULE ${SOURCE})
spec2def(untfs untfs.dll untfs.spec)

set_module_type(untfs nativedll)
target_link_libraries(untfs ntfslib)
add_importlibs(untfs ntdll)
add_cd_file(TARGET untfs DESTINATION reactos/system32 FOR all)
