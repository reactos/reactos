
add_executable(binpatch patch.c)
set_module_type(binpatch win32cui)
add_importlibs(binpatch ntdll msvcrt kernel32)
add_cd_file(TARGET binpatch DESTINATION reactos/bin FOR all)
