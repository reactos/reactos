
set_unicode()

spec2def(batt.dll batt.spec)

add_library(batt SHARED batt.c batt.rc ${CMAKE_CURRENT_BINARY_DIR}/batt.def)

set_module_type(batt win32dll)

add_importlibs(batt kernel32)

add_cab_target(batt 1)
