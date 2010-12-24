
set_unicode()

spec2def(serialui.dll serialui.spec)

list(APPEND SOURCE
    serialui.c
    serialui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/serialui.def)

add_library(serialui SHARED ${SOURCE})

set_module_type(serialui win32dll)
add_importlibs(serialui user32 shlwapi msvcrt kernel32)
add_cab_target(serialui 1)
