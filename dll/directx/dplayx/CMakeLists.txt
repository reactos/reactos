
set_rc_compiler()

spec2def(dplayx.dll dplayx.spec ADD_IMPORTLIB)

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
    ${CMAKE_CURRENT_BINARY_DIR}/dplayx.def)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(dplayx SHARED ${SOURCE})
set_module_type(dplayx win32dll)

target_link_libraries(dplayx
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

add_cd_file(TARGET dplayx DESTINATION reactos/system32 FOR all)

