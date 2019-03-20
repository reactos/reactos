
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mscoree.dll mscoree.spec)

list(APPEND SOURCE
    assembly.c
    config.c
    cordebug.c
    corruntimehost.c
    metadata.c
    metahost.c
    mscoree_main.c
    mscoree_private.h)

add_library(mscoree MODULE
    ${SOURCE}
    guid.c
    mscoree.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mscoree_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mscoree.def)

set_module_type(mscoree win32dll)
target_link_libraries(mscoree uuid wine)
add_importlibs(mscoree dbghelp advapi32 shell32 ole32 shlwapi msvcrt kernel32 ntdll)
add_pch(mscoree mscoree_private.h SOURCE)
add_cd_file(TARGET mscoree DESTINATION reactos/system32 FOR all)
