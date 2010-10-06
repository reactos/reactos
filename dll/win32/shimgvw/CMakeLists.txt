
spec2def(shimgvw ${CMAKE_CURRENT_SOURCE_DIR}/shimgvw.spec ${CMAKE_CURRENT_BINARY_DIR}/shimgvw.def)

add_library(shimgvw SHARED shimgvw.c shimgvw.rc)

set_module_type(shimgvw win32dll)

target_link_libraries(shimgvw
    ${CMAKE_CURRENT_BINARY_DIR}/shimgvw.def
    wine)

add_importlibs(shimgvw
    advapi32
    comctl32
    user32
    gdi32
    gdiplus
    kernel32
    ntdll)

add_dependencies(shimgvw shimgvw_def)
