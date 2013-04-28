
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(mscoree.dll mscoree.spec)

list(APPEND SOURCE
    assembly.c
    config.c
    cordebug.c
    corruntimehost.c
    metadata.c
    metahost.c
    mscoree_main.c
    mscoree.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mscoree_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mscoree.def)

add_library(mscoree SHARED ${SOURCE})
set_module_type(mscoree win32dll)
target_link_libraries(mscoree wine uuid)
add_importlibs(mscoree dbghelp advapi32 shell32 ole32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET mscoree DESTINATION reactos/system32 FOR all)
