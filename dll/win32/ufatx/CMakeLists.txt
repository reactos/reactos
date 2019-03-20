
spec2def(ufatx.dll ufatx.spec)

list(APPEND SOURCE
    ufatx.c
    ufatx.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ufatx.def)

add_library(ufatx MODULE ${SOURCE})
set_module_type(ufatx nativedll)
target_link_libraries(ufatx vfatxlib)
add_importlibs(ufatx ntdll)
add_cd_file(TARGET ufatx DESTINATION reactos/system32 FOR all)
