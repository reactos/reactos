# Source list mirrors dlls/dxdiagn/Makefile.in upstream; keep them in sync on resync.

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(dxdiagn.dll dxdiagn.spec)

list(APPEND SOURCE
    container.c
    dxdiag_main.c
    provider.c)

add_library(dxdiagn MODULE
    ${SOURCE}
    dxdiagn.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxdiagn.def)

add_idl_reg_scripts(dxdiagn registry dxdiagn.idl)

set_module_type(dxdiagn win32dll)
target_link_libraries(dxdiagn wine wine_dll_register wine_dll_canunload strmiids dxguid uuid oldnames)
add_importlibs(dxdiagn
    d3d9 ddraw dsound version ole32 oleaut32 user32 advapi32 psapi
    msvcrt kernel32 ntdll)
add_dependencies(dxdiagn wineheaders)
add_cd_file(TARGET dxdiagn DESTINATION reactos/system32 FOR all)
set_wine_module(dxdiagn)
