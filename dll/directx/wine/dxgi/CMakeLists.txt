

add_definitions(
    -D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

spec2def(dxgi.dll dxgi.spec ADD_IMPORTLIB)

add_library(dxgi MODULE
    version.rc
    adapter.c
    dxgi_main.c
    factory.c
    output.c
    surface.c
    swapchain.c
    device.c
    utils.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxgi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxgi.def)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(dxgi PRIVATE -Wno-sequence-point -Wno-unused-function -Wno-unused-but-set-variable -Wno-error) # Our favourite compiler :)
endif()

set_module_type(dxgi win32dll)
target_link_libraries(dxgi dxguid uuid wine)
add_importlibs(dxgi advapi32 gdi32 user32 d3dwine msvcrt kernel32 ntdll)
add_dependencies(dxgi wineheaders d3d_idl_headers)
add_cd_file(TARGET dxgi DESTINATION reactos/system32 FOR all)
