set_rc_compiler()

spec2def(ddraw.dll ddraw.spec)

list(APPEND SOURCE
    ddraw.rc
    main.c
    startup.c
    cleanup.c
    Ddraw/ddraw_main.c
    Ddraw/ddraw_displaymode.c
    Ddraw/ddraw_setcooperativelevel.c
    Ddraw/GetCaps.c
    Ddraw/GetDeviceIdentifier.c
    Ddraw/ddraw_stubs.c
    Ddraw/callbacks_dd_hel.c
    Surface/surface_stubs.c
    Surface/surface_main.c
    Surface/callbacks_surf_hel.c
    Surface/createsurface.c
    Clipper/clipper_stubs.c
    Clipper/clipper_main.c
    Color/color_stubs.c
    d3d/DirectD3D_main.c
    Gamma/gamma_stubs.c
    Kernel/kernel_stubs.c
    Palette/palette.c
    Palette/createpalette.c
    Videoport/videoport_stubs.c
    Vtable/DirectDraw7_Vtable.c
    Vtable/DirectDraw4_Vtable.c
    Vtable/DirectDraw2_Vtable.c
    Vtable/DirectDraw_Vtable.c
    Vtable/DirectDrawSurface7_Vtable.c
    Vtable/DirectDrawSurface4_Vtable.c
    Vtable/DirectDrawSurface3_Vtable.c
    Vtable/DirectDrawSurface2_Vtable.c
    Vtable/DirectDrawSurface_Vtable.c
    Vtable/DirectD3D_Vtable.c
    Vtable/DirectD3D2_Vtable.c
    Vtable/DirectD3D3_Vtable.c
    Vtable/DirectD3D7_Vtable.c)

add_library(ddraw SHARED ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/ddraw.def)

set_module_type(ddraw win32dll)

target_link_libraries(ddraw
    uuid
    dxguid
    ${PSEH_LIB})

add_importlibs(ddraw
    user32
    gdi32
    d3d8thk
    ole32
    advapi32
    msvcrt
    kernel32
    ntdll)
add_importlib_target(ddraw.spec)

add_cd_file(TARGET ddraw DESTINATION reactos/system32 FOR all)
