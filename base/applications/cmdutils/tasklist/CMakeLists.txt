
add_executable(tasklist tasklist.c tasklist.rc)
set_module_type(tasklist win32cui UNICODE)
target_link_libraries(tasklist conutils)
add_importlibs(tasklist msvcrt kernel32 ntdll)
add_cd_file(TARGET tasklist DESTINATION reactos/system32 FOR all)
