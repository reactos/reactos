project(SHELL)

set_cpp(WITH_RUNTIME)

add_definitions(
    -DUNICODE
    -D_UNICODE
    -D_ATL_NO_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    CChangeNotifyServer.cpp
    CDesktopBrowser.cpp
    CDirectoryWatcher.cpp
    CDirectoryList.cpp
    dde.cpp
    shelldesktop.h)

add_library(shelldesktop ${SOURCE})
add_pch(shelldesktop shelldesktop.h)
add_dependencies(shelldesktop xdk psdk)
