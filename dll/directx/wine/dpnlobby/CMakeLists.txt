
spec2def(dpnlobby.dll dpnlobby.spec)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dpnlobby_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dpnlobby.def)

add_library(dpnlobby MODULE ${SOURCE})
set_module_type(dpnlobby win32dll)
add_importlibs(dpnlobby dpnet msvcrt kernel32 ntdll)
add_cd_file(TARGET dpnlobby DESTINATION reactos/system32 FOR all)
set_wine_module(dpnlobby)
