
spec2def(mscat32.dll mscat32.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/mscat32.def
    ${CMAKE_CURRENT_BINARY_DIR}/mscat32_stubs.c)

add_library(mscat32 MODULE ${SOURCE})
set_module_type(mscat32 win32dll)
target_link_libraries(mscat32 wine)
add_importlibs(mscat32 wintrust msvcrt kernel32 ntdll)
add_cd_file(TARGET mscat32 DESTINATION reactos/system32 FOR all)
set_wine_module(mscat32)
