project(SHELL)

set_cpp(WITH_RUNTIME)

add_definitions(
    -DUNICODE
    -D_UNICODE
    -D_ATL_NO_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    CDesktopBrowser.cpp
    dde.cpp)

add_library(shelldesktop ${SOURCE})
add_dependencies(shelldesktop xdk psdk)
