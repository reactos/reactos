
list(APPEND SOURCE
    wshirda.c
    wshirda.rc)

add_library(wshirda MODULE ${SOURCE})
spec2def(wshirda wshirda.dll wshirda.spec)

set_module_type(wshirda win32dll UNICODE)
add_importlibs(wshirda ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wshirda DESTINATION reactos/system32 FOR all)
