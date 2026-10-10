
spec2def(hid.dll hid.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    hid.c
    precomp.h)

add_library(hid MODULE
    ${SOURCE}
    hid.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hid.def)
add_pch(hid precomp.h SOURCE)

set_module_type(hid win32dll UNICODE)
add_importlibs(hid msvcrt kernel32 ntdll)
target_link_libraries(hid hidparser_um)
add_cd_file(TARGET hid DESTINATION reactos/system32 FOR all)
