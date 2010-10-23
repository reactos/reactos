
add_subdirectory(notifyhook)

set_unicode()

#NOTE : explorer doesn't follow standard c++, and so doesn't compile with stlport headers
#I'm not willing to do it if explorer_new is hanging around.
#jgardou
#set_cpp()

add_definitions(-DWIN32)
add_definitions(-D__WINDRES__)

#add_pch(explorer ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h SOURCE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND SOURCE
    shell/mainframe.cpp
    shell/unixfs.cpp
    shell/ntobjfs.cpp
    shell/filechild.cpp
    shell/shellfs.cpp
    shell/fatfs.cpp
    shell/pane.cpp
    shell/regfs.cpp
    shell/webchild.cpp
    shell/entries.cpp
    shell/shellbrowser.cpp
    shell/winfs.cpp
    dialogs/searchprogram.cpp
    dialogs/settings.cpp
    i386-stub-win32.c
    taskbar/taskbar.cpp
    taskbar/favorites.cpp
    taskbar/quicklaunch.cpp
    taskbar/desktopbar.cpp
    taskbar/startmenu.cpp
    taskbar/traynotify.cpp
    precomp.cpp
    explorer.rc
    services/startup.c
    services/shellservices.cpp
    desktop/desktop.cpp
    explorer.cpp
    utility/xs-native.cpp
    utility/shellclasses.cpp
    utility/dragdropimpl.cpp
    utility/utility.cpp
    utility/xmlstorage.cpp
    utility/splitpath.c
    utility/window.cpp
    utility/shellbrowserimpl.cpp) #    utility/shelltests.cpp

add_executable(explorer ${SOURCE}) #${CMAKE_CURRENT_BINARY_DIR}/explorer_precomp.h.gch ${SOURCE})

set_subsystem(explorer windows)
set_entrypoint(explorer WinMainCRTStartup)

target_link_libraries(explorer
    -lsupc++
    -lstdc++
    -lgcc
    -lmingw32
    -lmingwex
    wine
    mingw_wmain
    mingw_common
    uuid
    -ladvapi32
    -lgdi32
    -luser32
    -lws2_32
    -lmsimg32
    -lcomctl32
    -lole32
    -loleaut32
    -lshell32
    -lnotifyhook
    -lmsvcrt
    -lkernel32
    -lntdll)

add_dependencies(explorer psdk buildno_header)
add_livecd_target(explorer reactos)