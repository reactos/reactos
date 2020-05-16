
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(printui.dll printui.spec)

list(APPEND SOURCE
    printui.c
    ${CMAKE_CURRENT_BINARY_DIR}/printui_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/printui.def)

add_library(printui MODULE ${SOURCE} printui.rc)
set_module_type(printui win32dll)
target_link_libraries(printui wine)
add_importlibs(printui shell32 msvcrt kernel32 ntdll)
add_cd_file(TARGET printui DESTINATION reactos/system32 FOR all)
