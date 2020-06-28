
spec2def(wzcsapi.dll wzcsapi.spec)

list(APPEND SOURCE
    wzcsapi.c
    wzcsapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wzcsapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wzcsapi.def)

add_library(wzcsapi MODULE ${SOURCE})
set_module_type(wzcsapi win32dll)
add_importlibs(wzcsapi msvcrt kernel32 ntdll)
add_cd_file(TARGET wzcsapi DESTINATION reactos/system32 FOR all)
