
add_rc_deps(menuaccels.rc ${CMAKE_CURRENT_SOURCE_DIR}/menuaccels.ico)
add_executable(menuaccels menuaccels.c menuaccels.rc)
set_module_type(menuaccels win32gui UNICODE)
add_importlibs(menuaccels user32 msvcrt kernel32)
add_rostests_file(TARGET menuaccels SUBDIR suppl)
