
list(APPEND SOURCE
    serialui.c
    serialui.rc)

add_library(serialui MODULE ${SOURCE})
spec2def(serialui serialui.dll serialui.spec)

set_module_type(serialui win32dll UNICODE)
add_importlibs(serialui user32 shlwapi msvcrt kernel32)
add_cd_file(TARGET serialui DESTINATION reactos/system32 FOR all)
