
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    d3dx9_25_main.c
    effect.c)

add_library(d3dx9_25 MODULE ${SOURCE} version.rc)
spec2def(d3dx9_25 d3dx9_25.dll d3dx9_25.spec)

set_module_type(d3dx9_25 win32dll)
target_link_libraries(d3dx9_25 wine)
add_importlibs(d3dx9_25 d3dx9_30 msvcrt kernel32 ntdll)
add_cd_file(TARGET d3dx9_25 DESTINATION reactos/system32 FOR all)
