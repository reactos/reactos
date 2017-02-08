
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(dxdiagn.dll dxdiagn.spec)
add_idl_headers(dxdiagn_idlheader fil_data.idl)

list(APPEND SOURCE
    container.c
    dxdiag_main.c
    provider.c
    dxdiag_private.h)

add_library(dxdiagn SHARED
    ${SOURCE}
    guid.c
    dxdiagn.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxdiagn.def)

set_module_type(dxdiagn win32dll)
target_link_libraries(dxdiagn strmiids dxguid uuid wine)
add_dependencies(dxdiagn dxdiagn_idlheader)
add_importlibs(dxdiagn d3d9 ddraw version ole32 oleaut32 psapi user32 dsound msvcrt kernel32 ntdll)
add_pch(dxdiagn dxdiag_private.h SOURCE)
add_cd_file(TARGET dxdiagn DESTINATION reactos/system32 FOR all)
