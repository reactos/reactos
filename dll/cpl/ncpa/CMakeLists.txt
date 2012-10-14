

spec2def(ncpa.cpl ncpa.spec)

add_library(ncpa SHARED ncpa.c ${CMAKE_CURRENT_BINARY_DIR}/ncpa.def)

set_module_type(ncpa cpl UNICODE)

add_importlibs(ncpa
    msvcrt
    advapi32
    shell32
    kernel32)

add_cd_file(TARGET ncpa DESTINATION reactos/system32 FOR all)
