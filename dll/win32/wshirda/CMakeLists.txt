
set_unicode()

spec2def(wshirda ${CMAKE_CURRENT_SOURCE_DIR}/wshirda.spec ${CMAKE_CURRENT_BINARY_DIR}/wshirda.def)

add_library(wshirda SHARED wshirda.c wshirda.rc)

set_module_type(wshirda win32dll)

target_link_libraries(wshirda ${CMAKE_CURRENT_BINARY_DIR}/wshirda.def)
add_importlibs(wshirda ws2_32 kernel32 ntdll)
add_dependencies(wshirda wshirda_def)
add_livecd_target(wshirda reactos/system32)
