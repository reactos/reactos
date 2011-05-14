
set_rc_compiler()
set_unicode()

add_executable(logon logon.c logon.rc)

set_module_type(logon win32gui)
set_target_properties(logon PROPERTIES SUFFIX ".scr")

target_link_libraries(logon scrnsave)
add_importlibs(logon user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET logon DESTINATION reactos/system32)
