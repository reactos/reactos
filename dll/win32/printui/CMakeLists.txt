
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    printui.c)

add_library(printui MODULE ${SOURCE} printui.rc)
spec2def(printui printui.dll printui.spec)

set_module_type(printui win32dll)
target_link_libraries(printui wine)
add_importlibs(printui shell32 msvcrt kernel32 ntdll)
add_cd_file(TARGET printui DESTINATION reactos/system32 FOR all)
