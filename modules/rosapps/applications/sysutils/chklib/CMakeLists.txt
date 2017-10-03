
add_executable(chklib chklib.c chklib.rc)
set_module_type(chklib win32cui)
target_link_libraries(chklib win32err)
add_importlibs(chklib msvcrt kernel32)
add_cd_file(TARGET chklib DESTINATION reactos/system32 FOR all)
