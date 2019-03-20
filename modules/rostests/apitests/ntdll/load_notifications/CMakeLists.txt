
add_library(load_notifications MODULE load_notifications.c)
set_module_type(load_notifications win32dll ENTRYPOINT DllMain 12)
add_importlibs(load_notifications kernel32 ntdll)
add_dependencies(load_notifications psdk)
add_rostests_file(TARGET load_notifications)
