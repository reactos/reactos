
add_executable(reg reg.c reg.rc)
set_module_type(reg win32cui UNICODE)
target_link_libraries(reg wine)
add_importlibs(reg advapi32 user32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET reg DESTINATION reactos/system32 FOR all)
