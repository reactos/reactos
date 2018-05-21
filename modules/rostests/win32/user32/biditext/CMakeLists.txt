
add_rc_deps(biditext.rc ${CMAKE_CURRENT_SOURCE_DIR}/Application.ico)
add_executable(biditext biditext.c biditext.rc)
set_module_type(biditext win32gui UNICODE)
add_importlibs(biditext lpk gdi32 user32 shell32 comctl32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET biditext SUBDIR suppl)
