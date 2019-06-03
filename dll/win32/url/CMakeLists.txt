
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    url_main.c)

add_library(url MODULE ${SOURCE})
spec2def(url url.dll url.spec)

set_module_type(url win32dll)
target_link_libraries(url wine)
add_importlibs(url shell32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET url DESTINATION reactos/system32 FOR all)
