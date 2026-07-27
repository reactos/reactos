
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(dpvoice.dll dpvoice.spec)

list(APPEND SOURCE
    client.c
    main.c
    server.c)

add_library(dpvoice MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dpvoice.def)

add_idl_reg_scripts(dpvoice registry dpvoice.idl)

set_module_type(dpvoice win32dll)
target_link_libraries(dpvoice wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dpvoice ole32 msvcrt kernel32 ntdll)
add_dependencies(dpvoice wineheaders)
add_cd_file(TARGET dpvoice DESTINATION reactos/system32 FOR all)
set_wine_module(dpvoice)
