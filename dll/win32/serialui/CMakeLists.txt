
spec2def(serialui.dll serialui.spec)

list(APPEND SOURCE
    serialui.c
    serialui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/serialui.def)

add_library(serialui MODULE ${SOURCE})
set_module_type(serialui win32dll UNICODE)
add_importlibs(serialui user32 shlwapi msvcrt kernel32)
add_cd_file(TARGET serialui DESTINATION reactos/system32 FOR all)
