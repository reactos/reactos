PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

add_definitions(-DUNICODE -D_UNICODE)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

list(APPEND SOURCE
    CBandSite.cpp
    CDesktopBrowser.cpp
    CMenuBand.cpp
    CMenuDeskBar.cpp
    CMenuFocusManager.cpp
    CMenuSite.cpp
    CMenuToolbars.cpp
    CMergedFolder.cpp
    CStartMenu.cpp
    ShellDDE.cpp)

add_library(shellmenu ${SOURCE})
