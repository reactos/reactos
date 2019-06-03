
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    credui_main.c)

add_library(credui MODULE ${SOURCE} credui.rc)
spec2def(credui credui.dll credui.spec ADD_IMPORTLIB)

set_module_type(credui win32dll)
target_link_libraries(credui wine)
add_importlibs(credui advapi32 user32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET credui DESTINATION reactos/system32 FOR all)
