
add_executable(runas runas.c runas.rc)
set_module_type(runas win32cui UNICODE)
target_link_libraries(runas conutils)
add_importlibs(runas advapi32 msvcrt kernel32)
add_cd_file(TARGET runas DESTINATION reactos/system32 FOR all)
