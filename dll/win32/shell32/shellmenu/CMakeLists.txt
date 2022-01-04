project(SHELL)

add_definitions(
    -DUNICODE
    -D_UNICODE)

list(APPEND SOURCE
    CMenuBand.cpp
    CMenuDeskBar.cpp
    CMenuFocusManager.cpp
    CMenuSite.cpp
    CMenuToolbars.cpp
    CMergedFolder.cpp
    CStartMenu.cpp)

add_library(shellmenu OBJECT ${SOURCE})
target_link_libraries(shellmenu PRIVATE atl_classes)
add_dependencies(shellmenu xdk psdk)
