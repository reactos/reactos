
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    compstui_main.c)

add_library(compstui MODULE ${SOURCE})
spec2def(compstui compstui.dll compstui.spec)

set_module_type(compstui win32dll)
target_link_libraries(compstui wine)
add_importlibs(compstui msvcrt kernel32 ntdll)
add_cd_file(TARGET compstui DESTINATION reactos/system32 FOR all)
