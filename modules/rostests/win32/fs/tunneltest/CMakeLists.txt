
list(APPEND SOURCE
    tunneltest.c)

add_executable(tunneltest ${SOURCE})
set_module_type(tunneltest win32cui UNICODE)
add_importlibs(tunneltest msvcrt kernel32 ntdll)
add_rostests_file(TARGET tunneltest SUBDIR suppl)
