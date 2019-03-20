
spec2def(ucdfs.dll ucdfs.spec)

list(APPEND SOURCE
    ucdfs.c
    ucdfs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ucdfs.def)

add_library(ucdfs MODULE ${SOURCE})
set_module_type(ucdfs nativedll)
target_link_libraries(ucdfs cdfslib)
add_importlibs(ucdfs ntdll)
add_cd_file(TARGET ucdfs DESTINATION reactos/system32 FOR all)
