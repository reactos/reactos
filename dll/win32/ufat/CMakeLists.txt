
list(APPEND SOURCE
    ufat.c
    ufat.rc)

add_library(ufat MODULE ${SOURCE})
spec2def(ufat ufat.dll ufat.spec)

set_module_type(ufat nativedll)
target_link_libraries(ufat vfatlib)
add_importlibs(ufat ntdll)
add_cd_file(TARGET ufat DESTINATION reactos/system32 FOR all)
