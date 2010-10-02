
add_subdirectory(notifyhook)

set_unicode()

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

target_link_libraries(explorer
    wine
    uuid)

set_module_type(explorer win32gui)

add_importlibs(explorer 
    advapi32
    gdi32
    user32
    ws2_32
    msimg32
    comctl32
    ole32
    oleaut32
    shell32
    notifyhook
    msvcrt)
