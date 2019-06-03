
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    d3dx9_38_main.c)

add_library(d3dx9_38 MODULE ${SOURCE} version.rc)
spec2def(d3dx9_38 d3dx9_38.dll d3dx9_38.spec)

set_module_type(d3dx9_38 win32dll)
target_link_libraries(d3dx9_38 wine)
add_importlibs(d3dx9_38 d3dx9_36 msvcrt kernel32 ntdll)
add_cd_file(TARGET d3dx9_38 DESTINATION reactos/system32 FOR all)
