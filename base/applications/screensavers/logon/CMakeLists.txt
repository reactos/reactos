
file(GLOB logon_rc_deps res/*.*)
add_rc_deps(logon.rc ${logon_rc_deps})
add_executable(logon logon.c logon.rc)
set_module_type(logon win32gui UNICODE)
set_target_properties(logon PROPERTIES SUFFIX ".scr")
target_link_libraries(logon scrnsave)
add_importlibs(logon user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET logon DESTINATION reactos/system32 FOR all)
