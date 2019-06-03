
list(APPEND SOURCE
    ufatx.c
    ufatx.rc)

add_library(ufatx MODULE ${SOURCE})
spec2def(ufatx ufatx.dll ufatx.spec)

set_module_type(ufatx nativedll)
target_link_libraries(ufatx vfatxlib)
add_importlibs(ufatx ntdll)
add_cd_file(TARGET ufatx DESTINATION reactos/system32 FOR all)
