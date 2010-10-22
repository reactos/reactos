
set_unicode()

spec2def(modemui ${CMAKE_CURRENT_SOURCE_DIR}/modemui.spec ${CMAKE_CURRENT_BINARY_DIR}/modemui.def)

add_library(modemui SHARED modemui.c modemui.rc)

set_module_type(modemui win32dll)

target_link_libraries(modemui ${CMAKE_CURRENT_BINARY_DIR}/modemui.def)

add_importlibs(modemui kernel32)
add_dependencies(modemui modemui_def)
add_livecd_target(modemui reactos/system32)
