
add_executable(mode mode.c mode.rc)
set_module_type(mode win32cui)
add_importlibs(mode shell32 user32 msvcrt kernel32)
add_cd_file(TARGET mode DESTINATION reactos/system32 FOR all)
