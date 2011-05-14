
set_unicode()

add_executable(lodctr lodctr_main.c)

set_module_type(lodctr win32cui)
add_importlibs(lodctr loadperf msvcrt kernel32)
add_cd_file(TARGET lodctr DESTINATION reactos/system32)
