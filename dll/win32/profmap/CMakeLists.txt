
spec2def(profmap.dll profmap.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    profmap_main.c
    profmap.rc
    ${CMAKE_CURRENT_BINARY_DIR}/profmap_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/profmap.def)

add_library(profmap MODULE ${SOURCE})
set_module_type(profmap win32dll)
add_importlibs(profmap userenv netapi32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET profmap DESTINATION reactos/system32 FOR all)
