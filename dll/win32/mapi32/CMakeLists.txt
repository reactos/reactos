
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(mapi32.dll mapi32.spec)

list(APPEND SOURCE
    imalloc.c
    mapi32_main.c
    prop.c
    sendmail.c
    util.c
    stubs.c
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/mapi32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mapi32.def)

add_library(mapi32 SHARED ${SOURCE} version.rc)
set_module_type(mapi32 win32dll)
target_link_libraries(mapi32 uuid wine)
add_importlibs(mapi32 shlwapi user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mapi32 DESTINATION reactos/system32 FOR all)
