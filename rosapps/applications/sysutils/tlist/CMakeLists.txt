
add_executable(tlist tlist.c tlist.rc)
set_module_type(tlist win32cui)
target_link_libraries(tlist epsapi getopt)
add_importlibs(tlist ntdll user32 msvcrt kernel32)
add_cd_file(TARGET tlist DESTINATION reactos/system32 FOR all)
