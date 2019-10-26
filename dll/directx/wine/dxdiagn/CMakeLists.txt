
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(dxdiagn.dll dxdiagn.spec)

list(APPEND SOURCE
    container.c
    dxdiag_main.c
    provider.c
    precomp.h)

add_library(dxdiagn MODULE
    ${SOURCE}
    guid.c
    dxdiagn.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxdiagn.def)

set_module_type(dxdiagn win32dll)
target_link_libraries(dxdiagn strmiids dxguid uuid wine)
add_dependencies(dxdiagn wineheaders)
add_importlibs(dxdiagn d3d9 ddraw version ole32 oleaut32 psapi user32 dsound msvcrt kernel32 ntdll)
add_pch(dxdiagn precomp.h SOURCE)
add_cd_file(TARGET dxdiagn DESTINATION reactos/system32 FOR all)
