
set_unicode()

add_library(batt SHARED batt.c batt.rc)

set_module_type(batt win32dll)

set_pdef_file(batt batt.pdef)
add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/batt.def)

add_importlibs(batt kernel32)

add_cab_target(batt 1)
