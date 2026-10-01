
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(d3dxof.dll d3dxof.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3dxof.c
    main.c
    mszip.c
    parsing.c)

add_library(d3dxof MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dxof.def)

add_idl_reg_scripts(d3dxof registry d3dxof.idl)

set_module_type(d3dxof win32dll)
target_link_libraries(d3dxof wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(d3dxof ole32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(d3dxof wineheaders)
add_cd_file(TARGET d3dxof DESTINATION reactos/system32 FOR all)
set_wine_module(d3dxof)
