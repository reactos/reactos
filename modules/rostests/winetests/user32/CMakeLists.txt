
remove_definitions(-DWINVER=0x502 -D_WIN32_WINNT=0x502)
add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -DNTDDI_VERSION=0x0A000005)

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
    wsprintf.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(user32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    resource.rc)

if(MSVC)
    # Disable warning C4005 (macro redefinition)
    target_compile_options(user32_winetest PRIVATE "/wd4005")
    # Disable warning C4477 (printf format warnings)
    remove_target_compile_option(user32_winetest "/we4477")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(user32_winetest PRIVATE -Wno-format)
endif()

add_pch(user32_winetest precomp.h "${PCH_SKIP_SOURCE}")
set_module_type(user32_winetest win32cui)
add_importlibs(user32_winetest user32 gdi32 advapi32 msvcrt kernel32 ntdll imm32)
add_rostests_file(TARGET user32_winetest)
target_compile_definitions(user32_winetest PRIVATE wcsicmp=_wcsicmp)
