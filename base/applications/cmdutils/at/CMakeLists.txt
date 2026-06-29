
add_executable(at at.c at.rc)
set_module_type(at win32cui UNICODE)
target_link_libraries(at conutils)
add_importlibs(at netapi32 msvcrt kernel32)
add_cd_file(TARGET at DESTINATION reactos/system32 FOR all)
