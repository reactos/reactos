
add_executable(netreg netreg.cpp netreg.rc)
target_link_libraries(netreg cppstl)
set_target_cpp_properties(netreg WITH_EXCEPTIONS)
set_module_type(netreg win32cui)
add_importlibs(netreg advapi32 user32 ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET netreg DESTINATION reactos/system32 FOR all)
