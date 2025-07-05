
add_definitions(
    -D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/vkd3d/include)
spec2def(dxva2.dll dxva2.spec ADD_IMPORTLIB)

add_library(dxva2 MODULE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxva2_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dxva2.def)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(dxva2 PRIVATE -Wno-sequence-point -Wno-unused-function -Wno-unused-but-set-variable -Wno-error) # Our favourite compiler :)
endif()

set_module_type(dxva2 win32dll)
target_link_libraries(dxva2 dx10guid dxguid uuid wine)
add_importlibs(dxva2 ole32 advapi32 gdi32 user32 d3dwine msvcrt kernel32_vista kernel32 ntdll)
add_dependencies(dxva2 wineheaders d3d_idl_headers)
add_cd_file(TARGET dxva2 DESTINATION reactos/system32 FOR all)
