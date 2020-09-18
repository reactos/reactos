project(SHELL)

add_definitions(
    -DUNICODE
    -D_UNICODE)


list(APPEND SOURCE
    CChangeNotifyServer.cpp
    CDesktopBrowser.cpp
    CDirectoryWatcher.cpp
    CDirectoryList.cpp
    dde.cpp)

add_library(shelldesktop OBJECT ${SOURCE})
target_link_libraries(shelldesktop PRIVATE atl_classes)
add_dependencies(shelldesktop xdk psdk)
