project(SHELL)

add_definitions(
    -DUNICODE
    -D_UNICODE)

if(DBG)
    add_definitions(-D_DEBUG=1) # CORE-17505
endif()

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
