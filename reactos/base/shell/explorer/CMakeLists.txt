
add_subdirectory(notifyhook)

set_cpp(WITH_RTTI WITH_EXCEPTIONS WITH_STL)

add_definitions(
    -DWIN32
    -D__WINDRES__)

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
    taskbar/taskbar.cpp
    taskbar/favorites.cpp
    taskbar/quicklaunch.cpp
    taskbar/desktopbar.cpp
    taskbar/startmenu.cpp
    taskbar/traynotify.cpp
    services/shellservices.cpp
    desktop/desktop.cpp
    explorer.cpp
    utility/xs-native.cpp
    utility/shellclasses.cpp
    utility/dragdropimpl.cpp
    utility/utility.cpp
    utility/xmlstorage.cpp
    utility/window.cpp
    utility/shellbrowserimpl.cpp
    precomp.h) #    utility/shelltests.cpp

if(ARCH STREQUAL "i386")
    list(APPEND I386_SOURCE i386-stub-win32.c)
endif()

add_executable(explorer_old
    ${SOURCE}
    ${I386_SOURCE}
    services/startup.c
    explorer.rc)

target_link_libraries(explorer_old comsupp wine uuid)
set_module_type(explorer_old win32gui UNICODE)
add_importlibs(explorer_old advapi32 gdi32 user32 ws2_32 msimg32 comctl32 ole32 oleaut32 shell32 shlwapi notifyhook msvcrt kernel32 ntdll)
add_pch(explorer_old precomp.h SOURCE)
add_dependencies(explorer_old psdk)
add_cd_file(TARGET explorer_old DESTINATION reactos FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/explorer-cfg-template.xml DESTINATION reactos FOR all)

if(NOT MSVC)
    add_compile_flags("-Wno-error=narrowing")

    # GCC bug #59472
    if(LTCG)
        add_target_link_flags(explorer_old "-Wno-error")
    endif()
endif()
