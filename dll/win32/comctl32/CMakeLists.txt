
set_unicode()

add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)
add_definitions(-D_COMCTL32_)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(comctl32.dll comctl32.spec)

add_library(comctl32 SHARED
    animate.c
    comboex.c
    comctl32undoc.c
    commctrl.c
    datetime.c
    dpa.c
    draglist.c
    dsa.c
    flatsb.c
    header.c
    hotkey.c
    imagelist.c
    ipaddress.c
    listview.c
    monthcal.c
    nativefont.c
    pager.c
    progress.c
    propsheet.c
    rebar.c
    smoothscroll.c
    status.c
    string.c
    syslink.c
    tab.c
    theme_button.c
    theme_combo.c
    theme_dialog.c
    theme_edit.c
    theme_listbox.c
    theming.c
    toolbar.c
    tooltips.c
    trackbar.c
    treeview.c
    updown.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/comctl32.def)

set_module_type(comctl32 win32dll)

target_link_libraries(comctl32
    uuid
    wine)

add_importlibs(comctl32
    user32
    gdi32
    advapi32
    winmm
    uxtheme
    msvcrt
    kernel32
    ntdll)

add_cab_target(comctl32 1)
add_importlib_target(comctl32.spec)
