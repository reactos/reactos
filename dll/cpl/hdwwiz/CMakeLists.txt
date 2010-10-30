
set_unicode()

spec2def(hdwwiz.cpl hdwwiz.spec)

add_library(hdwwiz SHARED hdwwiz.c hdwwiz.rc ${CMAKE_CURRENT_BINARY_DIR}/hdwwiz.def)

set_module_type(hdwwiz cpl)

add_importlibs(hdwwiz
    msvcrt
    setupapi
    advapi32
    comctl32
    rpcrt4
    user32
    gdi32
    devmgr
    kernel32
    ntdll)

add_cab_target(hdwwiz 1)
