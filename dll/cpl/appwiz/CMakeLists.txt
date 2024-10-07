
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__ -D_CRT_DECLARE_NONSTDC_NAMES=1)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(appwiz.cpl appwiz.spec)

list(APPEND SOURCE
    addons.c
    appwiz.c
    createlink.c
    removestartmenuitems.c
    appwiz.h)

file(GLOB appwiz_rc_deps resources/*.*)
add_rc_deps(appwiz.rc ${appwiz_rc_deps})

add_library(appwiz MODULE
    ${SOURCE}
    appwiz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/appwiz.def)

set_module_type(appwiz cpl UNICODE)
target_link_libraries(appwiz uuid wine)
add_delay_importlibs(appwiz msi)
add_importlibs(appwiz urlmon ole32 comctl32 advapi32 shell32 shlwapi user32 msvcrt kernel32 ntdll)
add_pch(appwiz appwiz.h SOURCE)
add_cd_file(TARGET appwiz DESTINATION reactos/system32 FOR all)
