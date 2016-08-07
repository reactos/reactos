
spec2def(input_new.dll input.spec)

list(APPEND SOURCE
    input.c
    input.h)

add_rc_deps(input.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/keyboard-shortcuts.ico)

add_library(input_new SHARED
    ${SOURCE}
    input.rc
    ${CMAKE_CURRENT_BINARY_DIR}/input_new.def)

set_module_type(input_new win32dll UNICODE)
add_importlibs(input_new advapi32 user32 comctl32 gdi32 msvcrt kernel32)
add_pch(input_new input.h SOURCE)
add_cd_file(TARGET input_new DESTINATION reactos/system32 FOR all)
