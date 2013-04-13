
add_definitions(
    -DCOM_NO_WINDOWS_H
    -D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

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
    ${CMAKE_CURRENT_BINARY_DIR}/dplayx.def)

add_library(dplayx SHARED ${SOURCE} version.rc)
set_module_type(dplayx win32dll)
target_link_libraries(dplayx dxguid uuid wine)
add_importlibs(dplayx winmm ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dplayx DESTINATION reactos/system32 FOR all)
