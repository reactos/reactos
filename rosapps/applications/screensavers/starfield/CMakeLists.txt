
add_executable(starfield screensaver.c starfield.rc)
set_module_type(starfield win32gui UNICODE)
set_target_properties(starfield PROPERTIES SUFFIX ".scr")
target_link_libraries(starfield scrnsave)
add_importlibs(starfield user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET starfield DESTINATION reactos/system32 FOR all)
