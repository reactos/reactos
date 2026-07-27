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

# dplayx.idl carries "#pragma makedep register": it is the COM registration
# data for DllRegisterServer, not a header. Must come after add_library():
# the helper appends to the existing target.
add_idl_reg_scripts(dplayx registry dplayx.idl)

set_module_type(dplayx win32dll)
# wine_dll_register/wine_dll_canunload supply the DllRegisterServer,
# DllUnregisterServer and DllCanUnloadNow exports declared in dplayx.spec;
# upstream Wine generates those from its own build glue.
# oldnames supplies the POSIX alias dplobby.c uses (strdup) plus the CRT
# opt-ins that declare it under MSVC.
target_link_libraries(dplayx wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dplayx winmm ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(dplayx wineheaders)
add_cd_file(TARGET dplayx DESTINATION reactos/system32 FOR all)
set_wine_module(dplayx)
