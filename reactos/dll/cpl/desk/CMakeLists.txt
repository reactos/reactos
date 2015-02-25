
add_definitions(-D_WIN32)
spec2def(desk.cpl desk.spec)

list(APPEND SOURCE
    advmon.c
    appearance.c
    background.c
    classinst.c
    desk.c
    devsett.c
    dibitmap.c
    misc.c
    preview.c
    screensaver.c
    advappdlg.c
    effappdlg.c
    settings.c
    monslctl.c
    general.c
    draw.c
    theme.c
    muireg.c
    desk.h)

add_library(desk SHARED
    ${SOURCE}
    guid.c
    desk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/desk.def)

set_module_type(desk cpl UNICODE)
target_link_libraries(desk uuid)
add_importlibs(desk user32 advapi32 gdi32 comctl32 comdlg32 ole32 setupapi shell32 shlwapi uxtheme gdiplus msvcrt kernel32 ntdll)
add_pch(desk desk.h SOURCE)
add_cd_file(TARGET desk DESTINATION reactos/system32 FOR all)
