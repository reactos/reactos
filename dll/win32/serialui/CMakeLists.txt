
set_unicode()

spec2def(serialui.dll serialui.spec)

add_library(serialui SHARED serialui.c serialui.rc ${CMAKE_CURRENT_BINARY_DIR}/serialui.def)

set_module_type(serialui win32dll)

add_importlibs(serialui user32 shlwapi kernel32)

add_cab_target(serialui 1)
