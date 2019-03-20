
spec2def(untfs.dll untfs.spec)

list(APPEND SOURCE
    untfs.c
    untfs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/untfs.def)

add_library(untfs MODULE ${SOURCE})
set_module_type(untfs nativedll)
target_link_libraries(untfs ntfslib)
add_importlibs(untfs ntdll)
add_cd_file(TARGET untfs DESTINATION reactos/system32 FOR all)
