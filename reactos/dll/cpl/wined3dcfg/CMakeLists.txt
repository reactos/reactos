
spec2def(wined3dcfg.cpl wined3dcfg.spec)

list(APPEND SOURCE
    wined3dcfg.c
    general.c
    wined3dcfg.h)

add_rc_deps(wined3dcfg.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/wined3dcfg.ico)

add_library(wined3dcfg SHARED
    ${SOURCE}
    wined3dcfg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wined3dcfg.def)

set_module_type(wined3dcfg cpl UNICODE)
add_importlibs(wined3dcfg user32 comctl32 advapi32 msvcrt kernel32)
add_pch(wined3dcfg wined3dcfg.h SOURCE)
add_cd_file(TARGET wined3dcfg DESTINATION reactos/system32 FOR all)
