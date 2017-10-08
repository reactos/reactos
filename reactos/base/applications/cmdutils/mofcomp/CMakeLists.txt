
add_executable(mofcomp mofcomp.c)
target_link_libraries(mofcomp wine)
set_module_type(mofcomp win32cui UNICODE)
add_importlibs(mofcomp msvcrt kernel32 ntdll)
add_cd_file(TARGET mofcomp DESTINATION reactos/system32/wbem FOR all)
