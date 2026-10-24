add_executable(user32_control_gallery
               main.c
               navhost.c
               pagehost.c
               pages/button.c
               pages/edit.c
               pages/combo.c
               pages/icontitle.c
               pages/listbox.c
               pages/mdi.c
               pages/scrollbar.c
               pages/static.c)

set_module_type(user32_control_gallery win32gui UNICODE)
add_importlibs(user32_control_gallery gdi32 user32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET user32_control_gallery SUBDIR suppl)
