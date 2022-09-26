
spec2def(input.dll input.spec)

list(APPEND SOURCE
    input.c
    input.h
    input_list.c
    input_list.h
    layout_list.c
    layout_list.h
    locale_list.c
    locale_list.h
    settings_page.c
    advanced_settings_page.c
    add_dialog.c
    edit_dialog.c
    key_settings_dialog.c
    key_sequence_dialog.c)

add_rc_deps(input.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/keyboard-shortcuts.ico)

add_library(input MODULE
    ${SOURCE}
    input.rc
    ${CMAKE_CURRENT_BINARY_DIR}/input.def)

set_module_type(input win32dll UNICODE)
add_importlibs(input advapi32 user32 comctl32 gdi32 msvcrt kernel32 setupapi)
add_pch(input input.h SOURCE)
add_cd_file(TARGET input DESTINATION reactos/system32 FOR all)
