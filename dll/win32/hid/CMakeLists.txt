
spec2def(hid.dll hid.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    hid.c
    precomp.h)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/hidparser)

add_library(hid MODULE
    ${SOURCE}
    hid.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hid.def)

set_module_type(hid win32dll UNICODE)
add_importlibs(hid msvcrt kernel32 ntdll)
target_link_libraries(hid hidparser_um)
add_pch(hid precomp.h SOURCE)
add_cd_file(TARGET hid DESTINATION reactos/system32 FOR all)
