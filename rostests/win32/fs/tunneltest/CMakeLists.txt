
list(APPEND SOURCE
    tunneltest.c)

add_executable(tunneltest ${SOURCE})
set_module_type(tunneltest win32cui UNICODE)
add_importlibs(tunneltest msvcrt kernel32 ntdll)
add_cd_file(TARGET tunneltest DESTINATION reactos/bin FOR all)
