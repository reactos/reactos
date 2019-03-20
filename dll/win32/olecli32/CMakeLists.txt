
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(olecli32.dll olecli32.spec)

list(APPEND SOURCE
    olecli_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/olecli32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/olecli32.def)

add_library(olecli32 MODULE ${SOURCE})
set_module_type(olecli32 win32dll)
target_link_libraries(olecli32 wine)
add_importlibs(olecli32 ole32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET olecli32 DESTINATION reactos/system32 FOR all)
