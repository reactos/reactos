
set_rc_compiler()
add_executable(hh main.c hh.rc)
set_module_type(hh win32gui)
add_importlibs(hh gdi32 user32 msvcrt kernel32)
add_cab_target(hh 4)
