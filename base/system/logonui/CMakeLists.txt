
add_executable(logonui logonui.c NT5design.c logonui.rc)
set_module_type(logonui win32gui UNICODE)
add_importlibs(logonui gdi32 user32 msimg32 shell32 msvcrt kernel32)
add_cd_file(TARGET logonui DESTINATION reactos/system32 FOR all)
