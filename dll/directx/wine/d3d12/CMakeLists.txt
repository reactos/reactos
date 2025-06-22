

add_definitions(
    -D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/vkd3d/include)

spec2def(d3d12.dll d3d12.spec ADD_IMPORTLIB)

add_library(d3d12 MODULE
     d3d12_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d12.def)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(d3d12 PRIVATE -Wno-sequence-point -Wno-unused-function -Wno-unused-but-set-variable -Wno-error) # Our favourite compiler :)
endif()

set_module_type(d3d12 win32dll)
target_link_libraries(d3d12 dxguid dx10guid uuid wine)
add_importlibs(d3d12 dxgi advapi32 gdi32 user32 d3dwine msvcrt kernel32 kernel32_vista ntdll)
add_dependencies(d3d12 wineheaders)
add_cd_file(TARGET d3d12 DESTINATION reactos/system32 FOR all)
