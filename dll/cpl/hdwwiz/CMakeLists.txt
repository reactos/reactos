
set_unicode()

spec2def(hdwwiz ${CMAKE_CURRENT_SOURCE_DIR}/hdwwiz.spec ${CMAKE_CURRENT_BINARY_DIR}/hdwwiz.def)

add_library(hdwwiz SHARED hdwwiz.c hdwwiz.rc)

set_module_type(hdwwiz cpl)

target_link_libraries(hdwwiz ${CMAKE_CURRENT_BINARY_DIR}/hdwwiz.def)

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

add_dependencies(hdwwiz hdwwiz_def)
add_cab_target(hdwwiz 1)
