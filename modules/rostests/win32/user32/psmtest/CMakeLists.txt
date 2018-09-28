
add_executable(psmtest psmtest.c)
set_module_type(psmtest win32gui UNICODE)
add_importlibs(psmtest lpk gdi32 user32 shell32 comctl32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET psmtest SUBDIR suppl)
