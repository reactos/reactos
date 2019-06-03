
list(APPEND SOURCE
    winfax.c
    winfax.rc)

add_library(winfax MODULE ${SOURCE})
spec2def(winfax winfax.dll winfax.spec)

set_module_type(winfax win32dll)
add_importlibs(winfax msvcrt kernel32 ntdll)
add_cd_file(TARGET winfax DESTINATION reactos/system32 FOR all)
