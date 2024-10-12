
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(credui.dll credui.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    credui_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/credui_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/credui.def)

add_library(credui MODULE ${SOURCE} credui.rc)
set_module_type(credui win32dll)
target_link_libraries(credui wine oldnames)
add_importlibs(credui advapi32 user32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET credui DESTINATION reactos/system32 FOR all)
