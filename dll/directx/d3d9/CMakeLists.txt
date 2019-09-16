
spec2def(d3d9.dll d3d9.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    adapter.c
    d3d9.c
    d3d9_baseobject.c
    d3d9_callbacks.c
    d3d9_caps.c
    d3d9_create.c
    d3d9_cursor.c
    d3d9_device.c
    d3d9_haldevice.c
    d3d9_helpers.c
    d3d9_impl.c
    d3d9_mipmap.c
    d3d9_puredevice.c
    d3d9_resource.c
    d3d9_swapchain.c
    d3d9_texture.c
    device.c
    format.c)

add_library(d3d9 MODULE
    ${SOURCE}
    d3d9.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d9_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d9.def)

set_module_type(d3d9 win32dll)
target_link_libraries(d3d9 dxguid uuid)
add_importlibs(d3d9 d3d8thk advapi32 version user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET d3d9 DESTINATION reactos/system32 FOR all)
