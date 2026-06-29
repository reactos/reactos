
add_library(empty_dll MODULE empty_dll.c)
set_module_type(empty_dll win32dll ENTRYPOINT DllMain 12)
add_importlibs(empty_dll kernel32 ntdll)
add_dependencies(empty_dll psdk)
add_rostests_file(TARGET empty_dll)
