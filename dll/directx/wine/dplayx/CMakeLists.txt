# Source list mirrors dlls/dplayx/Makefile.in upstream; keep them in sync on resync.

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -DCOM_NO_WINDOWS_H)

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
    name_server.c)

add_library(dplayx MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dplayx.def)

add_idl_reg_scripts(dplayx registry dplayx.idl)

set_module_type(dplayx win32dll)
target_link_libraries(dplayx wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dplayx winmm ole32 user32 advapi32 kernel32_vista msvcrt kernel32 ntdll)
add_dependencies(dplayx wineheaders)
add_cd_file(TARGET dplayx DESTINATION reactos/system32 FOR all)
set_wine_module(dplayx)
