
set_cpp(WITH_RUNTIME WITH_STL)
add_executable(netreg netreg.cpp netreg.rc)
set_module_type(netreg win32cui)
add_importlibs(netreg advapi32 user32 ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET netreg DESTINATION reactos/system32 FOR all)
