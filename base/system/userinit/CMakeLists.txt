
add_executable(userinit userinit.c userinit.rc)
target_link_libraries(userinit wine)
set_module_type(userinit win32gui UNICODE)
add_importlibs(userinit user32 advapi32 shell32 msvcrt kernel32 ntdll)
add_cd_file(TARGET userinit DESTINATION reactos/system32 FOR all)
