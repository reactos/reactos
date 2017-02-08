
add_executable(extrac32 extrac32.c)
target_link_libraries(extrac32 wine)
set_module_type(extrac32 win32gui UNICODE)
add_importlibs(extrac32 shell32 setupapi shlwapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET extrac32 DESTINATION reactos/system32 FOR all)
