
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(dplay.dll dplay.spec)

add_library(dplay MODULE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dplay_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dplay.def)

set_module_type(dplay win32dll)
add_importlibs(dplay dplayx msvcrt kernel32)
add_cd_file(TARGET dplay DESTINATION reactos/system32 FOR all)
