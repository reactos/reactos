
set_unicode()

add_definitions(-D_WIN32)
add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(desk ${CMAKE_CURRENT_SOURCE_DIR}/desk.spec ${CMAKE_CURRENT_BINARY_DIR}/desk.def)

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
    theme.c)

set_module_type(desk cpl)

target_link_libraries(desk ${CMAKE_CURRENT_BINARY_DIR}/desk.def uuid)

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

add_dependencies(desk desk_def)
