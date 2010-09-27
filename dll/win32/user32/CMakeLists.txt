


add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-D_DLL -D__USE_CRTIMP)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)
include_directories(include)

set_rc_compiler()

list(APPEND SOURCE
    user32_stubs.c
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
    misc/ddeserver.c
    misc/desktop.c
    misc/display.c
    misc/dllmain.c
    misc/exit.c
    misc/exticon.c
    misc/imm.c
    misc/misc.c
    misc/object.c
    misc/resources.c
    misc/rtlstr.c
    misc/stubs.c
    misc/timer.c
    misc/usrapihk.c
    misc/winhelp.c
    misc/winsta.c
    misc/wsprintf.c
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
    windows/winpos.c)

add_library(user32 SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/user32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/user32_user32.h.gch)

set_target_properties(user32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(user32
    ${CMAKE_CURRENT_SOURCE_DIR}/user32.def
    wine
    -lgdi32
    -ladvapi32
    -limm32
    win32ksys
    pseh
    -lkernel32
    -lntdll)
                      
add_pch(user32 ${CMAKE_CURRENT_SOURCE_DIR}/include/user32.h ${SOURCE})
add_dependencies(user32 psdk)
