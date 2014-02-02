
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(d3dx9_43.dll d3dx9_43.spec)

list(APPEND SOURCE
    d3dx9_43_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx9_43.def)

add_library(d3dx9_43 SHARED ${SOURCE} version.rc)
set_module_type(d3dx9_43 win32dll)
add_importlibs(d3dx9_43 d3dx9_36 msvcrt kernel32)
add_cd_file(TARGET d3dx9_43 DESTINATION reactos/system32 FOR all)
