
add_subdirectory(notifyhook)

set_rc_compiler()

#NOTE : explorer doesn't follow standard c++, and so doesn't compile with stlport headers
#I'm not willing to do it if explorer_new is hanging around.
#jgardou
#set_cpp()

add_definitions(
    -DWIN32
    -D__WINDRES__
    -DUNICODE -D_UNICODE)

#add_pch(explorer precomp.h)
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
    utility/window.cpp
    utility/shellbrowserimpl.cpp) #    utility/shelltests.cpp

add_executable(explorer ${SOURCE})

set_subsystem(explorer windows)
set_entrypoint(explorer wWinMainCRTStartup)

target_link_libraries(explorer
    -lsupc++
    -lstdc++
    -lgcc
    -lmingwex
    -lmingw32
    wine
    uuid)

add_importlibs(explorer advapi32 gdi32 user32 ws2_32 msimg32 comctl32 ole32 oleaut32 shell32 shlwapi notifyhook msvcrt kernel32 ntdll)
set_image_base(explorer 0x00400000)
add_dependencies(explorer psdk)
add_cd_file(TARGET explorer DESTINATION reactos FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/explorer-cfg-template.xml DESTINATION reactos FOR all)
