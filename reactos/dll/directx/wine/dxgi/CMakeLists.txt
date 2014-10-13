
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dxgi.dll dxgi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    adapter.c
    device.c
    dxgi_main.c
    factory.c
    output.c
    surface.c
    swapchain.c
    utils.c
    dxgi_private.h)

add_library(dxgi SHARED
    ${SOURCE}
    guid.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxgi.def)

set_module_type(dxgi win32dll)
target_link_libraries(dxgi uuid dxgi_uuids wine)
add_importlibs(dxgi d3dwine user32 msvcrt kernel32 ntdll)
add_dependencies(dxgi wineheaders)
add_pch(dxgi dxgi_private.h SOURCE)
add_cd_file(TARGET dxgi DESTINATION reactos/system32 FOR all)
