# Mirrors dlls/d3d12core/Makefile.in upstream, which declares a MODULE and
# nothing else: the whole DLL is the two stubs in d3d12core.spec
# (D3D12GetInterface, D3D12SDKVersion). It exists so that applications shipping
# the Agility SDK redistributable find something to load.

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(d3d12core.dll d3d12core.spec)

add_library(d3d12core MODULE
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12core_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12core.def)

set_module_type(d3d12core win32dll)
target_link_libraries(d3d12core wine)
add_importlibs(d3d12core msvcrt kernel32 ntdll)
add_dependencies(d3d12core wineheaders)
add_cd_file(TARGET d3d12core DESTINATION reactos/system32 FOR all)
set_wine_module(d3d12core)
