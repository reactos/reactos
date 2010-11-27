
set_unicode()

add_library(serialui SHARED serialui.c serialui.rc serialui.def)

if(NOT MSVC)
    set_source_files_properties(serialui.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(serialui win32dll)

add_importlibs(serialui user32 shlwapi msvcrt kernel32)

add_cab_target(serialui 1)
