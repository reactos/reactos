
set_unicode()

add_library(kbsdll SHARED kbsdll.c kbsdll.rc)

spec2def(kbsdll ${CMAKE_CURRENT_SOURCE_DIR}/kbsdll.spec ${CMAKE_CURRENT_BINARY_DIR}/kbsdll.def)

target_link_libraries(kbsdll ${CMAKE_CURRENT_BINARY_DIR}/kbsdll.def)

set_module_type(kbsdll win32dll)
add_importlibs(kbsdll user32 comctl32 kernel32)

add_dependencies(kbsdll kbsdll_def)
add_livecd_target(kbsdll reactos/system32)
