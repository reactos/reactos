# Source list mirrors dlls/d3d12/Makefile.in upstream; keep them in sync on resync.
#
# This is the thin bridge from the D3D12 entry points onto vkd3d's D3D12
# runtime. It builds and links here, but it can only actually do anything on a
# machine with a Vulkan ICD: vkd3d_create_instance goes looking for vulkan-1.dll
# at run time and fails cleanly when there is none, which is the situation on
# stock ReactOS today.

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${VKD3D_INCLUDE_DIRS})

spec2def(d3d12.dll d3d12.spec ADD_IMPORTLIB)

add_library(d3d12 MODULE
    d3d12_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12.def)

set_module_type(d3d12 win32dll)
# Unlike d3dcompiler, this one DOES link the vkd3d static library: it needs the
# D3D12 runtime API (vkd3d_create_instance, vkd3d_create_device and friends),
# and wined3d only re-exports the vkd3d-shader half.
# dx10guid carries the D3D12 IIDs (and IID_IDXGIFactory4, IID_ID3D10Blob):
# vkd3d references them throughout, plain dxguid stops at D3D9.
target_link_libraries(d3d12 wine vkd3d dxguid dx10guid uuid)
add_importlibs(d3d12 dxgi d3dwine gdi32 user32 msvcrt kernel32 ntdll)
add_dependencies(d3d12 wineheaders d3d_idl_headers)
add_cd_file(TARGET d3d12 DESTINATION reactos/system32 FOR all)
set_wine_module(d3d12)
