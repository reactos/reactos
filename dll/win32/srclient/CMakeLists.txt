
spec2def(srclient.dll srclient.spec)

list(APPEND SOURCE
    srclient_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/srclient_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/srclient.def)

add_library(srclient MODULE ${SOURCE})
set_module_type(srclient win32dll)
add_importlibs(srclient msvcrt kernel32 ntdll)
add_cd_file(TARGET srclient DESTINATION reactos/system32 FOR all)
