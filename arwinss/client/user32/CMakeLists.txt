add_definitions(-D__WINESRC__
                -D_WINE)

remove_definitions(-DWINVER=0x502
                   -D_WIN32_WINNT=0x502
                   -D_WIN32_WINDOWS=0x502
                   -D_SETUPAPI_VER=0x502)

add_definitions(-DWINVER=0x0600
                -D_WIN32_WINNT=0x0600
                -D_WIN32_WINDOWS=0x600
                -D_SETUPAPI_VER=0x600)

spec2def(user32.dll user32.spec ADD_IMPORTLIB)

include_directories(
    BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/arwinss/include
    ${REACTOS_SOURCE_DIR}/win32ss/include
    )

set_rc_compiler()

list(APPEND SOURCE
    atom.c
    button.c
    caret.c
    class.c
    clipboard.c
    combo.c
    csr.c
    cursoricon.c
    dde_client.c
    dde_misc.c
    dde_server.c
    defdlg.c
    defwnd.c
    desktop.c
    dialog.c
    driver.c
    edit.c
    exticon.c
    focus.c
    hook.c
    icontitle.c
    input.c
    legacy.c
    listbox.c
    lstr.c
    mdi.c
    menu.c
    message.c
    misc.c
    msgbox.c
    nonclient.c
    painting.c
    property.c
    resource.c
    scroll.c
    spy.c
    static.c
    sysparams.c
    text.c
    uitools.c
    user_main.c
    win.c
    winhelp.c
    winpos.c
    winproc.c
    winstation.c
    wsprintf.c
    resources/user32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/user32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/user32.def)

add_library(user32 SHARED ${SOURCE})
set_module_type(user32 win32dll UNICODE)

target_link_libraries(user32
    user32_wsprintf
    wine
    win32ksys
    ${PSEH_LIB})

add_delay_importlibs(user32 imm32 usp10)
add_importlibs(user32 gdi32 advapi32 version msvcrt kernel32 ntdll)
add_pch(user32 include/user32.h)
add_cd_file(TARGET user32 DESTINATION reactos/system32 FOR all)

