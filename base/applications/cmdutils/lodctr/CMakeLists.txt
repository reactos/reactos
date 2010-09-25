add_executable(lodctr lodctr_main.c)

set_module_type(lodctr win32cui)

set_unicode(lodctr yes)

add_importlibs(lodctr loadperf msvcrt)