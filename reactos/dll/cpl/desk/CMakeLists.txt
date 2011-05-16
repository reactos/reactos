
set_unicode()

add_definitions(
    -D_WIN32
    -D_DLL -D__USE_CRTIMP)

set_rc_compiler()
spec2def(desk.cpl desk.spec)

add_library(desk SHARED
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
    desk.rc
    draw.c
    theme.c
    ${CMAKE_CURRENT_BINARY_DIR}/desk.def)

set_module_type(desk cpl)

target_link_libraries(desk uuid)

add_importlibs(desk
    msvcrt
    user32
    advapi32
    gdi32
    comctl32
    comdlg32
    ole32
    setupapi
    shell32
    msimg32
    kernel32
    ntdll)

add_cd_file(TARGET desk DESTINATION reactos/system32 FOR all)
