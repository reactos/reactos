
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dplay.dll dplay.spec)

add_library(dplay SHARED
    dplay_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dplay.def)

set_module_type(dplay win32dll UNICODE)
add_importlibs(dplay dplayx msvcrt kernel32 ntdll)
add_dependencies(dplay psdk)
add_cd_file(TARGET dplay DESTINATION reactos/system32 FOR all)
