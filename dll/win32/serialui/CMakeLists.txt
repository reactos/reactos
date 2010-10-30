
set_unicode()

spec2def(serialui ${CMAKE_CURRENT_SOURCE_DIR}/serialui.spec ${CMAKE_CURRENT_BINARY_DIR}/serialui.def)

add_library(serialui SHARED serialui.c serialui.rc)

set_module_type(serialui win32dll)

target_link_libraries(serialui ${CMAKE_CURRENT_BINARY_DIR}/serialui.def)

add_importlibs(serialui user32 shlwapi kernel32)
add_dependencies(serialui serialui_def)
add_cab_target(serialui 1)
