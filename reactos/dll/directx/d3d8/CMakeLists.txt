set_rc_compiler()

spec2def(d3d8.dll d3d8.spec)

list(APPEND SOURCE
    d3d8.c)

add_library(d3d8 SHARED ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/d3d8.def)

set_module_type(d3d8 win32dll)

add_importlibs(d3d8
    user32
    gdi32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET d3d8 DESTINATION reactos/system32 FOR all)
