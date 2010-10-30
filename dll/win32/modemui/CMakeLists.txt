
set_unicode()

spec2def(modemui.dll modemui.spec)

add_library(modemui SHARED modemui.c modemui.rc ${CMAKE_CURRENT_BINARY_DIR}/modemui.def)

set_module_type(modemui win32dll)

add_importlibs(modemui kernel32)

add_cab_target(modemui 1)
