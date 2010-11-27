
spec2def(shimgvw.dll shimgvw.spec)

add_library(shimgvw SHARED shimgvw.c shimgvw.rc ${CMAKE_CURRENT_BINARY_DIR}/shimgvw.def)

set_module_type(shimgvw win32dll)

target_link_libraries(shimgvw wine)

add_importlibs(shimgvw
    advapi32
    comctl32
    user32
    gdi32
    gdiplus
    msvcrt
    kernel32
    ntdll)


add_cab_target(shimgvw 1)
