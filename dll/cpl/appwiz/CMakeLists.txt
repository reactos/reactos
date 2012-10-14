
set_rc_compiler()

spec2def(appwiz.cpl appwiz.spec)

add_library(appwiz SHARED
    appwiz.c
    createlink.c
    appwiz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/appwiz.def)

set_module_type(appwiz cpl UNICODE)

target_link_libraries(appwiz uuid)

add_importlibs(appwiz
    msvcrt
    user32
    ole32
    advapi32
    comctl32
    shell32
    kernel32)

add_pch(appwiz appwiz.h)
add_cd_file(TARGET appwiz DESTINATION reactos/system32 FOR all)
