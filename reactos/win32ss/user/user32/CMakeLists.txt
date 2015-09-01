
spec2def(user32.dll user32.spec ADD_IMPORTLIB)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    include
    ${REACTOS_SOURCE_DIR}/win32ss/include)

list(APPEND SOURCE
    controls/appswitch.c
    controls/button.c
    controls/combo.c
    controls/edit.c
    controls/icontitle.c
    controls/listbox.c
    controls/regcontrol.c
    controls/scrollbar.c
    controls/static.c
    misc/dde.c
    misc/ddeclient.c
    misc/ddemisc.c
    misc/ddeserver.c
    misc/desktop.c
    misc/display.c
    misc/dllmain.c
    misc/exit.c
    misc/exticon.c
    misc/imm.c
    misc/logon.c
    misc/misc.c
    misc/object.c
    misc/resources.c
    misc/rtlstr.c
    misc/stubs.c
    misc/timer.c
    misc/usrapihk.c
    misc/winhelp.c
    misc/winsta.c
    windows/accel.c
    windows/caret.c
    windows/class.c
    windows/clipboard.c
    windows/cursoricon.c
    windows/dc.c
    windows/defwnd.c
    windows/dialog.c
    windows/draw.c
    windows/font.c
    windows/hook.c
    windows/input.c
    windows/mdi.c
    windows/menu.c
    windows/messagebox.c
    windows/message.c
    windows/nonclient.c
    windows/paint.c
    windows/prop.c
    windows/rect.c
    windows/spy.c
    windows/text.c
    windows/window.c
    windows/winpos.c
    ${REACTOS_SOURCE_DIR}/win32ss/user/rtl/text.c
    ${CMAKE_CURRENT_BINARY_DIR}/user32_stubs.c
    include/user32.h)

add_library(user32 SHARED
    ${SOURCE}
    user32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/user32.def)

set_module_type(user32 win32dll ENTRYPOINT DllMain 12 UNICODE HOTPATCHABLE)
target_link_libraries(user32 user32_wsprintf wine win32ksys ${PSEH_LIB})

if(MSVC)
    # for __ftol2_sse, float to int cast helper
    target_link_libraries(user32 msvcrtex)
endif()

add_delay_importlibs(user32 imm32 usp10)
add_importlibs(user32 gdi32 advapi32 kernel32 ntdll)
add_pch(user32 include/user32.h SOURCE)
add_cd_file(TARGET user32 DESTINATION reactos/system32 FOR all)
