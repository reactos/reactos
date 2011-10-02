
add_definitions(
    -D__ROS_LONG64__)

set_rc_compiler()

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
    testlist.c
    resource.rc)

add_executable(user32_winetest ${SOURCE})
target_link_libraries(user32_winetest wine)
set_module_type(user32_winetest win32cui)
add_importlibs(user32_winetest user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET user32_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    add_target_compile_flags(user32_winetest "-Wno-format")
endif()