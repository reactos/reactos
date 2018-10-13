
add_executable(messagebox messagebox.c messagebox.rc)
set_module_type(messagebox win32cui UNICODE)
add_importlibs(messagebox user32 msvcrt kernel32)
add_rostests_file(TARGET messagebox SUBDIR suppl)
