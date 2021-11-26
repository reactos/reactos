
spec2def(ddraw.dll ddraw.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cleanup.c
    main.c
    startup.c
    Clipper/clipper_main.c
    Clipper/clipper_stubs.c
    Color/color_stubs.c
    d3d/DirectD3D_main.c
    Ddraw/callbacks_dd_hel.c
    Ddraw/ddraw_displaymode.c
    Ddraw/ddraw_main.c
    Ddraw/ddraw_setcooperativelevel.c
    Ddraw/ddraw_stubs.c
    Ddraw/GetCaps.c
    Ddraw/GetDeviceIdentifier.c
    Gamma/gamma_stubs.c
    Kernel/kernel_stubs.c
    Palette/createpalette.c
    Palette/palette.c
    Surface/callbacks_surf_hel.c
    Surface/createsurface.c
    Surface/surface_main.c
    Surface/surface_stubs.c
    Videoport/videoport_stubs.c
    Vtable/DirectD3D_Vtable.c
    Vtable/DirectD3D2_Vtable.c
    Vtable/DirectD3D3_Vtable.c
    Vtable/DirectD3D7_Vtable.c
    Vtable/DirectDraw_Vtable.c
    Vtable/DirectDraw2_Vtable.c
    Vtable/DirectDraw4_Vtable.c
    Vtable/DirectDraw7_Vtable.c
    Vtable/DirectDrawSurface_Vtable.c
    Vtable/DirectDrawSurface2_Vtable.c
    Vtable/DirectDrawSurface3_Vtable.c
    Vtable/DirectDrawSurface4_Vtable.c
    Vtable/DirectDrawSurface7_Vtable.c)

add_library(ddraw MODULE
    ${SOURCE}
    ddraw.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ddraw.def)

set_module_type(ddraw win32dll)
target_link_libraries(ddraw uuid dxguid ${PSEH_LIB})
add_importlibs(ddraw advapi32 gdi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ddraw DESTINATION reactos/system32 FOR all)
