
add_executable(wmic main.c wmic.rc)
target_link_libraries(wmic wine)
set_module_type(wmic win32cui UNICODE)
add_importlibs(wmic oleaut32 ole32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wmic DESTINATION reactos/system32/wbem FOR all)
