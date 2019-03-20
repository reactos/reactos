
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(inseng.dll inseng.spec)

list(APPEND SOURCE
    icif.c
    inf.c
    inseng_main.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/inseng_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/inseng.def)

add_library(inseng MODULE ${SOURCE} guid.c inseng.rc)
set_module_type(inseng win32dll)
target_link_libraries(inseng uuid wine)
add_importlibs(inseng ole32 urlmon kernel32_vista msvcrt kernel32 ntdll)
add_pch(inseng precomp.h SOURCE)
add_cd_file(TARGET inseng DESTINATION reactos/system32 FOR all)
