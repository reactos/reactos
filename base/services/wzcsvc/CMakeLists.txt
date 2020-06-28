spec2def(wzcsvc.dll wzcsvc.spec ADD_IMPORTLIB)

add_library(wzcsvc MODULE
    wzcsvc.c
    wzcsvc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wzcsvc_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wzcsvc.def)

set_module_type(wzcsvc win32dll UNICODE)
add_importlibs(wzcsvc advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wzcsvc DESTINATION reactos/system32 FOR all)
