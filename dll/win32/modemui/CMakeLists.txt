
set_unicode()

spec2def(modemui.dll modemui.spec)

list(APPEND SOURCE
    modemui.c
    modemui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/modemui.def)

add_library(modemui SHARED ${SOURCE})

set_module_type(modemui win32dll)

add_importlibs(modemui msvcrt kernel32 ntdll)

add_cab_target(modemui 1)
