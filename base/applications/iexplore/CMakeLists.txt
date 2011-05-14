
set_rc_compiler()
add_executable(iexplore main.c iexplore.rc)
set_module_type(iexplore win32gui)
add_importlibs(iexplore shdocvw msvcrt kernel32)
add_cd_file(TARGET iexplore DESTINATION reactos)
