
add_executable(dialog_initvisible dialog_initvisible.c dialog_initvisible.rc)
set_module_type(dialog_initvisible win32gui UNICODE)
add_importlibs(dialog_initvisible user32 msvcrt kernel32)
add_rostests_file(TARGET dialog_initvisible SUBDIR suppl)
