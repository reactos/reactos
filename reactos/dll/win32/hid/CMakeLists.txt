

list(APPEND SOURCE
    hid.c
    stubs.c
    hid.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hid.def)

spec2def(hid.dll hid.spec)

add_library(hid SHARED ${SOURCE})

set_module_type(hid win32dll UNICODE)

add_importlibs(hid msvcrt kernel32 ntdll)
add_pch(hid precomp.h)
add_cd_file(TARGET hid DESTINATION reactos/system32 FOR all)
