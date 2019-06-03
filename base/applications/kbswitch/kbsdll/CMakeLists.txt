
list(APPEND SOURCE
    kbsdll.c
    kbsdll.rc)

add_library(kbsdll MODULE ${SOURCE})
spec2def(kbsdll kbsdll.dll kbsdll.spec)

set_module_type(kbsdll win32dll UNICODE)
add_importlibs(kbsdll user32 comctl32 msvcrt kernel32)
add_cd_file(TARGET kbsdll DESTINATION reactos/system32 FOR all)
