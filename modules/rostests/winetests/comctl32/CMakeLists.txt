
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
    updown.c)

list(APPEND PCH_SKIP_SOURCE
    button.c
    subclass.c
    testlist.c)

add_executable(comctl32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc)

if(MSVC AND ARCH STREQUAL "amd64")
    # warning C4334: '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    target_compile_options(comctl32_winetest PRIVATE /wd4334)
endif()

if(MSVC)
    #  warning C4045: 'large_truncated_80_w': array bounds overflow
    target_compile_options(comctl32_winetest PRIVATE /wd4045)
endif()

target_compile_options(comctl32_winetest PRIVATE $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wno-format>)

set_module_type(comctl32_winetest win32cui)
add_importlibs(comctl32_winetest comctl32 ole32 user32 gdi32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(comctl32_winetest ntdll)
endif()

add_pch(comctl32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET comctl32_winetest)
