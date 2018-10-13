
add_executable(dllhost dllhost.c dllhost.rc)
set_module_type(dllhost win32gui UNICODE)
add_importlibs(dllhost msvcrt kernel32 ntdll)
add_cd_file(TARGET dllhost DESTINATION reactos/system32 FOR all)
