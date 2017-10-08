
add_executable(write write.c rsrc.rc)
set_module_type(write win32gui UNICODE)
add_importlibs(write user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET write DESTINATION reactos/system32 FOR all)
