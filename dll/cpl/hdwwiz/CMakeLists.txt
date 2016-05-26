
spec2def(hdwwiz.cpl hdwwiz.spec)
file(GLOB hdwwiz_rc_deps resources/*.*)
add_rc_deps(hdwwiz.rc ${hdwwiz_rc_deps})

add_library(hdwwiz SHARED
    hdwwiz.c
    hdwwiz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hdwwiz.def)

set_module_type(hdwwiz cpl UNICODE)
add_importlibs(hdwwiz setupapi advapi32 comctl32 rpcrt4 user32 gdi32 devmgr msvcrt kernel32 ntdll)
add_cd_file(TARGET hdwwiz DESTINATION reactos/system32 FOR all)
