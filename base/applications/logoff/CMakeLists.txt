
add_executable(logoff logoff.c logoff.rc)
set_module_type(logoff win32cui UNICODE)
target_link_libraries(logoff conutils)
add_importlibs(logoff user32 msvcrt kernel32)
add_cd_file(TARGET logoff DESTINATION reactos/system32 FOR all)
