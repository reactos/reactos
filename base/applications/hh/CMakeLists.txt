
add_rc_deps(hh.rc ${CMAKE_CURRENT_SOURCE_DIR}/hh.ico)
add_executable(hh main.c hh.rc)
set_module_type(hh win32gui)
add_importlibs(hh gdi32 user32 msvcrt kernel32)
add_cd_file(TARGET hh DESTINATION reactos FOR all)
