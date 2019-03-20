
spec2def(uext2.dll uext2.spec)

list(APPEND SOURCE
    uext2.c
    uext2.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uext2.def)

add_library(uext2 MODULE ${SOURCE})
set_module_type(uext2 nativedll)
target_link_libraries(uext2 ext2lib)
add_importlibs(uext2 ntdll)
add_cd_file(TARGET uext2 DESTINATION reactos/system32 FOR all)
