
set_rc_compiler()

spec2def(wined3dcfg.cpl wined3dcfg.spec)

add_library(wined3dcfg SHARED
    wined3dcfg.c
    general.c
    wined3dcfg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wined3dcfg.def)

set_module_type(wined3dcfg cpl UNICODE)

add_importlibs(wined3dcfg
    msvcrt
    user32
    comctl32
    advapi32
    kernel32)

add_pch(wined3dcfg wined3dcfg.h)
add_cd_file(TARGET wined3dcfg DESTINATION reactos/system32 FOR all)
