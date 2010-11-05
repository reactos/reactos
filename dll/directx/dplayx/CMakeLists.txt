set_rc_compiler()

spec2def(dplayx.dll dplayx.spec)

list(APPEND SOURCE
    dpclassfactory.c
    dplay.c
    dplaysp.c
    dplayx_global.c
    dplayx_main.c
    dplayx_messages.c
    dplobby.c
    lobbysp.c
    name_server.c
    regsvr.c
    version.rc
    dplayx.def)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(dplayx SHARED ${SOURCE})
set_module_type(dplayx win32dll)

target_link_libraries(dplayx
    ${CMAKE_CURRENT_BINARY_DIR}/dplayx.def
    wine
    uuid
    dxguid)

add_importlibs(dplayx
    advapi32
    ole32
    user32
    winmm
    msvcrt
    kernel32
    ntdll)
    