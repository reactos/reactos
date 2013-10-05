
list(APPEND SOURCE
    broadcast.c
    class.c
    clipboard.c
    combo.c
    cursoricon.c
    dce.c
    dde.c
    dialog.c
    edit.c
    generated.c
    input.c
    listbox.c
    menu.c
    monitor.c
    msg.c
    resource.c
    scroll.c
    static.c
    sysparams.c
    text.c
    uitools.c
    win.c
    winstation.c
    wsprintf.c
    testlist.c)

add_executable(user32_winetest ${SOURCE} resource.rc)
set_module_type(user32_winetest win32cui)
add_importlibs(user32_winetest user32 gdi32 advapi32 msvcrt kernel32)
add_cd_file(TARGET user32_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #add_target_compile_flags(user32_winetest "-Wno-format")
    #allow_warnings(user32_winetest)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-format")
endif()
