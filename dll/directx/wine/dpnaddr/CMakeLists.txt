
spec2def(dpnaddr.dll dpnaddr.spec)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dpnaddr_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dpnaddr.def)

add_library(dpnaddr MODULE ${SOURCE})
set_module_type(dpnaddr win32dll)
add_importlibs(dpnaddr dpnet msvcrt kernel32 ntdll)
add_cd_file(TARGET dpnaddr DESTINATION reactos/system32 FOR all)
set_wine_module(dpnaddr)
