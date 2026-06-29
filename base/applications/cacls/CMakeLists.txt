
add_executable(cacls cacls.c cacls.rc)
set_module_type(cacls win32cui UNICODE)
target_link_libraries(cacls conutils)
add_importlibs(cacls advapi32 msvcrt kernel32)
add_cd_file(TARGET cacls DESTINATION reactos/system32 FOR all)
