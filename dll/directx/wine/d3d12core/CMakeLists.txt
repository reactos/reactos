
add_definitions(
    -D__WINESRC__
    -DUSE_WIN32_OPENGL)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(d3d12core.dll d3d12core.spec ADD_IMPORTLIB)

add_library(d3d12core SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12core_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12core.def)

set_module_type(d3d12core win32dll)
target_link_libraries(d3d12core wine uuid)
add_dependencies(d3d12core d3d_idl_headers)
add_importlibs(d3d12core d3dwine d3d12 msvcrt dxgi kernel32 ntdll)
add_cd_file(TARGET d3d12core DESTINATION reactos/system32 FOR all)
