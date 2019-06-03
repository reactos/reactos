
add_library(ncpa MODULE
    ncpa.c)
spec2def(ncpa ncpa.cpl ncpa.spec)

set_module_type(ncpa cpl UNICODE)
add_importlibs(ncpa advapi32 shell32 msvcrt kernel32)
add_cd_file(TARGET ncpa DESTINATION reactos/system32 FOR all)
