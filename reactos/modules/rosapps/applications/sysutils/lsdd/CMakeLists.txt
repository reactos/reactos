
add_executable(lsdd lsdd.c lsdd.rc)
set_module_type(lsdd win32cui)
target_link_libraries(lsdd win32err)
add_importlibs(lsdd user32 msvcrt kernel32)
add_cd_file(TARGET lsdd DESTINATION reactos/system32 FOR all)
