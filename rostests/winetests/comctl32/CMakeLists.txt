
remove_definitions(-D_WIN32_WINNT=0x502 -D_WIN32_IE=0x600)

add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    animate.c
    button.c
    comboex.c
    datetime.c
    dpa.c
    header.c
    imagelist.c
    ipaddress.c
    listview.c
    misc.c
    monthcal.c
    mru.c
    pager.c
    progress.c
    propsheet.c
    rebar.c
    status.c
    subclass.c
    syslink.c
    tab.c
    toolbar.c
    tooltips.c
    trackbar.c
    treeview.c
    updown.c
    testlist.c)

add_executable(comctl32_winetest ${SOURCE} rsrc.rc)
set_module_type(comctl32_winetest win32cui)
add_importlibs(comctl32_winetest comctl32 ole32 user32 gdi32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET comctl32_winetest)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #add_target_compile_flags(comctl32_winetest "-Wno-format")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-format")
endif()
