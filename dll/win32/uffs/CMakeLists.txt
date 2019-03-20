
spec2def(uffs.dll uffs.spec)

list(APPEND SOURCE
    uffs.c
    uffs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uffs.def)

add_library(uffs MODULE ${SOURCE})
set_module_type(uffs nativedll)
target_link_libraries(uffs ffslib)
add_importlibs(uffs ntdll)
add_cd_file(TARGET uffs DESTINATION reactos/system32 FOR all)
