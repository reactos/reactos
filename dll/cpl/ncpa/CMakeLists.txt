
spec2def(ncpa.cpl ncpa.spec)

add_library(ncpa MODULE
    ncpa.c
    ${CMAKE_CURRENT_BINARY_DIR}/ncpa.def)

set_module_type(ncpa cpl UNICODE)
add_importlibs(ncpa advapi32 shell32 msvcrt kernel32)
add_cd_file(TARGET ncpa DESTINATION reactos/system32 FOR all)
