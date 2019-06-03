
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    localui.c)

add_library(localui MODULE ${SOURCE} localui.rc)
spec2def(localui localui.dll localui.spec)

set_module_type(localui win32dll)
target_link_libraries(localui wine)
add_importlibs(localui winspool user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET localui DESTINATION reactos/system32 FOR all)
