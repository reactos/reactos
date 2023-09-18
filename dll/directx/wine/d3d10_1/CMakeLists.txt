
add_definitions(
    -D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

spec2def(d3d10_1.dll d3d10_1.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3d10_1_main.c)

add_library(d3d10_1 SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10_1_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d10_1.def)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(d3d10_1 PRIVATE -Wno-incompatible-pointer-types -Wno-error) # Our favourite compiler :)
endif()

set_module_type(d3d10_1 win32dll)
target_link_libraries(d3d10_1 wine dxguid uuid)
add_dependencies(d3d10_1 wineheaders d3d_idl_headers)
add_importlibs(d3d10_1 msvcrt kernel32 ntdll dxgi d3d10core d3d10)
add_cd_file(TARGET d3d10_1 DESTINATION reactos/system32 FOR all)
