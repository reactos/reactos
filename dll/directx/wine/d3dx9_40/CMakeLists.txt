
spec2def(d3dx9_40.dll d3dx9_40.spec)

list(APPEND SOURCE
    d3dx9_40_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx9_40.def)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_library(d3dx9_40 SHARED ${SOURCE})
set_module_type(d3dx9_40 win32dll)
target_link_libraries(d3dx9_40 wine)

add_importlibs(d3dx9_40
    d3dx9_36
    d3d9
    user32
    gdi32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_dependencies(d3dx9_40 wineheaders)
add_cd_file(TARGET d3dx9_40 DESTINATION reactos/system32 FOR all)
