
remove_definitions(-D_WIN32_WINNT=0x502 -D_WIN32_IE=0x600)

add_definitions(-DUSE_WINE_TODOS -DWINETEST_USE_DBGSTR_LONGLONG)

list(APPEND SOURCE
    animate.c
    combo.c
    datetime.c
    dpa.c
    edit.c
    header.c
    imagelist.c
    ipaddress.c
    listbox.c
    listview.c
    misc.c
    monthcal.c
    mru.c
    pager.c
    progress.c
    propsheet.c
    rebar.c
    static.c
    status.c
    syslink.c
    tab.c
    taskdialog.c
    toolbar.c
    tooltips.c
    trackbar.c
    treeview.c
    updown.c
    precomp.h)

add_executable(comctl32_winetest
    ${SOURCE}
    button.c
    subclass.c
    testlist.c
    rsrc.rc)

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(comctl32_winetest PRIVATE "-Wno-format")
endif()

set_module_type(comctl32_winetest win32cui)
add_importlibs(comctl32_winetest comctl32 ole32 user32 gdi32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(comctl32_winetest ntdll)
endif()

add_pch(comctl32_winetest precomp.h SOURCE)
add_rostests_file(TARGET comctl32_winetest)
