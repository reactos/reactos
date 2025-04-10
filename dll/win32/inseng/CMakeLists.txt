
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -Dstrcasecmp=_stricmp
    -Dstrncasecmp=_strnicmp
)
spec2def(inseng.dll inseng.spec)

list(APPEND SOURCE
    icif.c
    inf.c
    inseng_main.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(inseng MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    inseng.rc
    ${CMAKE_CURRENT_BINARY_DIR}/inseng_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/inseng.def)

set_module_type(inseng win32dll)
target_link_libraries(inseng uuid wine)
add_importlibs(inseng ole32 urlmon kernel32_vista msvcrt kernel32 ntdll)
add_pch(inseng precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET inseng DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(inseng) # CORE-5743: No CONST_VTABLE
