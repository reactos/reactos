
add_executable(kill kill.c kill.rc)
set_module_type(kill win32cui)
add_importlibs(kill ntdll user32 msvcrt kernel32)
add_cd_file(TARGET kill DESTINATION reactos/system32 FOR all)
