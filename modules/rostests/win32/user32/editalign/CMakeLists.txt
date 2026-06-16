
add_rc_deps(editalign.rc ${CMAKE_CURRENT_SOURCE_DIR}/editalign.ico)
add_executable(editalign editalign.c editalign.rc)
set_module_type(editalign win32gui UNICODE)
add_importlibs(editalign gdi32 user32 comctl32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET editalign SUBDIR suppl)
