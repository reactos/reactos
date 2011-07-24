spec2def(d3d9.dll d3d9.spec)

list(APPEND SOURCE
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
    adapter.c
    device.c
    format.c
    d3d9.rc)

add_library(d3d9 SHARED ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/d3d9.def)

set_module_type(d3d9 win32dll)

target_link_libraries(d3d9 uuid dxguid)

add_importlibs(d3d9
    advapi32
    user32
    gdi32
    version
    d3d8thk
    msvcrt
    kernel32
    ntdll)
add_importlib_target(d3d9.spec)

add_cd_file(TARGET d3d9 DESTINATION reactos/system32 FOR all)
