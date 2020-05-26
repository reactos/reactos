
remove_definitions(-DWINVER=0x502 -D_WIN32_WINNT=0x502)
add_definitions(-DWINVER=0x602 -D_WIN32_WINNT=0x602)

if(MSVC)
    # Disable warning C4477 (printf format warnings)
    add_compile_flags("/wd4477")
elseif(GCC AND GCC_VERSION VERSION_GREATER 7)
    add_compile_flags("-Wno-format-overflow")
endif()

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
    rawinput.c
    resource.c
    scroll.c
    static.c
    sysparams.c
    text.c
    uitools.c
    win.c
    winstation.c
    wsprintf.c
    precomp.h)

add_executable(user32_winetest
    ${SOURCE}
    testlist.c
    resource.rc)

add_pch(user32_winetest precomp.h SOURCE)
# some tests need to be run from an app compiled as GUI
set_module_type(user32_winetest win32gui)
add_importlibs(user32_winetest user32 gdi32 advapi32 msvcrt kernel32)
add_rostests_file(TARGET user32_winetest)
