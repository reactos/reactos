
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(d3dx9_32.dll d3dx9_32.spec)

list(APPEND SOURCE
    d3dx9_32_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx9_32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx9_32.def)

add_library(d3dx9_32 MODULE ${SOURCE} version.rc)
set_module_type(d3dx9_32 win32dll)
target_link_libraries(d3dx9_32 wine)
add_importlibs(d3dx9_32 d3dx9_36 msvcrt kernel32 ntdll)
add_cd_file(TARGET d3dx9_32 DESTINATION reactos/system32 FOR all)
