
add_executable(genguid genguid.c genguid.rc)
set_module_type(genguid win32cui)
target_link_libraries(genguid uuid)
add_importlibs(genguid ole32 msvcrt kernel32)
add_cd_file(TARGET genguid DESTINATION reactos/system32 FOR all)
