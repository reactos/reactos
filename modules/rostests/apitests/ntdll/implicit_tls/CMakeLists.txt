
add_library(implicit_tls MODULE implicit_tls.c)
set_module_type(implicit_tls win32dll ENTRYPOINT DllMain 12)
add_importlibs(implicit_tls kernel32 ntdll)
add_dependencies(implicit_tls psdk)
add_rostests_file(TARGET implicit_tls)
