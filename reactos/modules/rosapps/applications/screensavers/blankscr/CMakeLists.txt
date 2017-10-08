
add_executable(blankscr scrnsave.c scrnsave.rc)
set_module_type(blankscr win32gui UNICODE)
set_target_properties(blankscr PROPERTIES SUFFIX ".scr")
target_link_libraries(blankscr scrnsave)
add_importlibs(blankscr user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET blankscr DESTINATION reactos/system32 FOR all)
