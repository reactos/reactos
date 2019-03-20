
spec2def(ufat.dll ufat.spec)

list(APPEND SOURCE
    ufat.c
    ufat.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ufat.def)

add_library(ufat MODULE ${SOURCE})
set_module_type(ufat nativedll)
target_link_libraries(ufat vfatlib)
add_importlibs(ufat ntdll)
add_cd_file(TARGET ufat DESTINATION reactos/system32 FOR all)
