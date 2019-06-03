
list(APPEND SOURCE
    modemui.c
    modemui.rc)

add_library(modemui MODULE ${SOURCE})
spec2def(modemui modemui.dll modemui.spec)

set_module_type(modemui win32dll UNICODE)
add_importlibs(modemui msvcrt kernel32 ntdll)
add_cd_file(TARGET modemui DESTINATION reactos/system32 FOR all)
