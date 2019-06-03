
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    icif.c
    inf.c
    inseng_main.c
    precomp.h)

add_library(inseng MODULE ${SOURCE} guid.c inseng.rc)
spec2def(inseng inseng.dll inseng.spec)

set_module_type(inseng win32dll)
target_link_libraries(inseng uuid wine)
add_importlibs(inseng ole32 urlmon kernel32_vista msvcrt kernel32 ntdll)
add_pch(inseng precomp.h SOURCE)
add_cd_file(TARGET inseng DESTINATION reactos/system32 FOR all)
