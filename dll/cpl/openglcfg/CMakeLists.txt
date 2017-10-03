
spec2def(openglcfg.cpl openglcfg.spec)

list(APPEND SOURCE
    openglcfg.c
    general.c
    openglcfg.h)

add_rc_deps(openglcfg.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/openglcfg.ico)

add_library(openglcfg SHARED
    ${SOURCE}
    openglcfg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/openglcfg.def)

set_module_type(openglcfg cpl UNICODE)
add_importlibs(openglcfg user32 comctl32 advapi32 msvcrt kernel32)
add_pch(openglcfg openglcfg.h SOURCE)
add_cd_file(TARGET openglcfg DESTINATION reactos/system32 FOR all)
