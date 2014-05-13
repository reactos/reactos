
add_executable(tee tee.c tee.rc)
set_module_type(tee win32cui)
add_importlibs(tee msvcrt kernel32)
add_cd_file(TARGET tee DESTINATION reactos/system32 FOR all)
