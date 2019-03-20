
spec2def(modemui.dll modemui.spec)

list(APPEND SOURCE
    modemui.c
    modemui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/modemui_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/modemui.def)

add_library(modemui MODULE ${SOURCE})
set_module_type(modemui win32dll UNICODE)
add_importlibs(modemui msvcrt kernel32 ntdll)
add_cd_file(TARGET modemui DESTINATION reactos/system32 FOR all)
