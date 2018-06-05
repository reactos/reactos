
add_executable(mshta main.c mshta.rc)
target_link_libraries(mshta)
set_module_type(mshta win32gui)
add_importlibs(mshta msvcrt mshtml kernel32 ntdll)
add_cd_file(TARGET mshta DESTINATION reactos/system32 FOR all)
