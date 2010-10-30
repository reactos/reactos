
set_unicode()

spec2def(wshirda.dll wshirda.spec)

add_library(wshirda SHARED wshirda.c wshirda.rc ${CMAKE_CURRENT_BINARY_DIR}/wshirda.def)

set_module_type(wshirda win32dll)

add_importlibs(wshirda ws2_32 kernel32 ntdll)

add_cab_target(wshirda 1)
