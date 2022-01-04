project(rapps)

add_executable(rapps.com main.c rapps.rc)
set_module_type(rapps.com win32cui UNICODE)
set_target_properties(rapps.com PROPERTIES SUFFIX "")
add_importlibs(rapps.com msvcrt kernel32 ntdll)
add_cd_file(TARGET rapps.com DESTINATION reactos/system32 FOR all)
