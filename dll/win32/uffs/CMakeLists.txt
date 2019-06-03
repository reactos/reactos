
list(APPEND SOURCE
    uffs.c
    uffs.rc)

add_library(uffs MODULE ${SOURCE})
spec2def(uffs uffs.dll uffs.spec)

set_module_type(uffs nativedll)
target_link_libraries(uffs ffslib)
add_importlibs(uffs ntdll)
add_cd_file(TARGET uffs DESTINATION reactos/system32 FOR all)
