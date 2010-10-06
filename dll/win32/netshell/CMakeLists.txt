
add_definitions(-D_NETSHELL_)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(netshell ${CMAKE_CURRENT_SOURCE_DIR}/netshell.spec ${CMAKE_CURRENT_BINARY_DIR}/netshell.def)

list(APPEND SOURCE
    netshell.c
    shfldr_netconnect.c
    enumlist.c
    netshell.rc
    classfactory.c
    connectmanager.c
    lanconnectui.c
    lanstatusui.c)

add_library(netshell SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/netshell_precomp.h.gch
    ${SOURCE})

set_module_type(netshell win32dll)

target_link_libraries(netshell
    ${CMAKE_CURRENT_BINARY_DIR}/netshell.def
    uuid
    wine)

add_importlibs(netshell
    msvcrt
    shlwapi
    shell32
    version
    iphlpapi
    ole32
    user32
    advapi32
    setupapi
    ws2_32
    comctl32
    kernel32)

add_pch(netshell ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(netshell netshell_def)
