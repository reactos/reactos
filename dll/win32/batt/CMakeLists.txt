
set_unicode()

spec2def(batt ${CMAKE_CURRENT_SOURCE_DIR}/batt.spec ${CMAKE_CURRENT_BINARY_DIR}/batt.def)

add_library(batt SHARED batt.c batt.rc)

set_module_type(batt win32dll)

target_link_libraries(batt ${CMAKE_CURRENT_BINARY_DIR}/batt.def)

add_importlibs(batt kernel32)
add_dependencies(batt batt_def)
add_livecd_target(batt reactos/system32)
