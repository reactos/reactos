
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(msimg32.dll msimg32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    msimg32_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/msimg32.def)

add_library(msimg32 MODULE ${SOURCE})
set_module_type(msimg32 win32dll)
target_link_libraries(msimg32 wine)
add_importlibs(msimg32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msimg32 DESTINATION reactos/system32 FOR all)
