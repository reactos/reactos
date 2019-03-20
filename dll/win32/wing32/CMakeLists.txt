
spec2def(wing32.dll wing32.spec)

list(APPEND SOURCE
    wing32.c
    ${CMAKE_CURRENT_BINARY_DIR}/wing32.def)

add_library(wing32 MODULE ${SOURCE})
set_module_type(wing32 win32dll UNICODE)
add_importlibs(wing32 user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET wing32 DESTINATION reactos/system32 FOR all)
