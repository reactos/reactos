
add_executable(thmsvc thmsvc.c thmsvc.rc)
target_link_libraries(thmsvc wine)
set_module_type(thmsvc win32cui UNICODE)
add_importlibs(thmsvc uxtheme advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET thmsvc DESTINATION reactos/system32 FOR all)
