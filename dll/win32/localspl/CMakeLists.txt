
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(localspl.dll localspl.spec)

list(APPEND SOURCE
    localmon.c
    localspl_main.c
    provider.c
    localspl_private.h)

add_library(localspl SHARED
    ${SOURCE}
    localspl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/localspl_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/localspl.def)

set_module_type(localspl win32dll)
target_link_libraries(localspl wine)
add_importlibs(localspl spoolss user32 advapi32 advapi32_vista msvcrt kernel32 ntdll)
add_pch(localspl localspl_private.h SOURCE)
add_cd_file(TARGET localspl DESTINATION reactos/system32 FOR all)
