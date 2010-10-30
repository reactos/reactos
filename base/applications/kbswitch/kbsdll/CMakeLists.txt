
set_unicode()

add_library(kbsdll SHARED kbsdll.c kbsdll.rc ${CMAKE_CURRENT_BINARY_DIR}/kbsdll.def)

spec2def(kbsdll.dll kbsdll.spec)

set_module_type(kbsdll win32dll)
add_importlibs(kbsdll user32 comctl32 kernel32)

add_cab_target(kbsdll 1)
