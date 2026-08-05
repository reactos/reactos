
# dpnet_main.c guards its one-time init with InitOnceExecuteOnce.
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x600
    -DWINVER=0x600)

spec2def(dpnet.dll dpnet.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    address.c
    client.c
    dpnet_main.c
    lobbiedapp.c
    peer.c
    server.c
    threadpool.c)

add_library(dpnet MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dpnet.def)

add_idl_reg_scripts(dpnet registry dpnet.idl)

# version.rc pulls in dpnet.rgs, which registers the winsock service provider.
set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/dpnet.rgs")

set_module_type(dpnet win32dll)
target_link_libraries(dpnet wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dpnet ws2_32 ole32 advapi32 kernel32_vista ucrtbase msvcrt kernel32 ntdll)
add_dependencies(dpnet wineheaders)
add_cd_file(TARGET dpnet DESTINATION reactos/system32 FOR all)
set_wine_module(dpnet)
