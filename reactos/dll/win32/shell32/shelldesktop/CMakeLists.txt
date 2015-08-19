PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

add_definitions(-DUNICODE -D_UNICODE)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

list(APPEND SOURCE
    CDesktopBrowser.cpp
    dde.cpp)

add_library(shelldesktop ${SOURCE})
