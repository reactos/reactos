
add_executable(logonui logonui.c logonui.rc NT5design.c)
set_module_type(logonui win32gui UNICODE)
add_importlibs(logonui user32 shell32 gdi32 msimg32 msvcrt kernel32)
add_cd_file(TARGET logonui DESTINATION reactos/system32 FOR all)
