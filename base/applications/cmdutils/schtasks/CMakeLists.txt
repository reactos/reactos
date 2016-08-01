
add_executable(schtasks schtasks.c)
target_link_libraries(schtasks wine)
set_module_type(schtasks win32cui UNICODE)
add_importlibs(schtasks msvcrt kernel32 ntdll)
add_cd_file(TARGET schtasks DESTINATION reactos/system32 FOR all)
