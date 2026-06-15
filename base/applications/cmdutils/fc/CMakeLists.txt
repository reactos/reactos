
add_executable(fc fc.c texta.c textw.c fc.rc)
set_module_type(fc win32cui UNICODE)
target_link_libraries(fc conutils)
add_importlibs(fc shlwapi msvcrt kernel32)
add_cd_file(TARGET fc DESTINATION reactos/system32 FOR all)
