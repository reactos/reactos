
spec2def(winfax.dll winfax.spec)

list(APPEND SOURCE
    winfax.c
    winfax.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winfax.def)

add_library(winfax MODULE ${SOURCE})
set_module_type(winfax win32dll)
add_importlibs(winfax msvcrt kernel32 ntdll)
add_cd_file(TARGET winfax DESTINATION reactos/system32 FOR all)
