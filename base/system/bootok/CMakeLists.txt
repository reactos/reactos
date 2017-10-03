
add_executable(bootok bootok.c bootok.rc)
set_module_type(bootok win32cui UNICODE)
add_importlibs(bootok advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET bootok DESTINATION reactos/system32 FOR all)
