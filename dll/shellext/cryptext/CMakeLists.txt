
list(APPEND SOURCE
    cryptext.c
    precomp.h)

add_library(cryptext MODULE
    ${SOURCE}
    cryptext.spec
    cryptext.rc)
spec2def(cryptext cryptext.dll cryptext.spec)

set_module_type(cryptext win32dll UNICODE)
target_link_libraries(cryptext uuid)
add_importlibs(cryptext cryptui crypt32 user32 msvcrt kernel32 ntdll)
add_pch(cryptext precomp.h SOURCE)
add_cd_file(TARGET cryptext DESTINATION reactos/system32 FOR all)
