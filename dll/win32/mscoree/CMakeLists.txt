
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DUSE_NEW_WINE_REGISTER_RESOURCES)

spec2def(mscoree.dll mscoree.spec)

list(APPEND SOURCE
    assembly.c
    config.c
    cordebug.c
    corruntimehost.c
    metadata.c
    metahost.c
    mscoree_main.c
    reactos.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/mscoree_stubs.c)

add_library(mscoree MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    mscoree.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mscoree.def)

set_module_type(mscoree win32dll)
target_link_libraries(mscoree uuid wine oldnames)
add_importlibs(mscoree dbghelp advapi32 shell32 kernel32_vista ole32 shlwapi msvcrt kernel32 ntdll)
#add_pch(mscoree mscoree_private.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET mscoree DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mscoree) # CORE-5743: No CONST_VTABLE
