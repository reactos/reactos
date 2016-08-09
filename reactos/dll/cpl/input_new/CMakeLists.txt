PROJECT(INPUT_NEW)

spec2def(input_new.dll input.spec)

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

add_library(input_new SHARED
    ${SOURCE}
    input.rc
    ${CMAKE_CURRENT_BINARY_DIR}/input_new.def)

set_module_type(input_new win32dll UNICODE)
add_importlibs(input_new advapi32 user32 comctl32 gdi32 msvcrt kernel32 setupapi)
add_pch(input_new input.h SOURCE)
add_cd_file(TARGET input_new DESTINATION reactos/system32 FOR all)
