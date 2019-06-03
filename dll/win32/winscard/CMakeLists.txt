
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    winscard.c
    stubs.c
    precomp.h)

add_library(winscard MODULE
    ${SOURCE}
    winscard.rc)
spec2def(winscard winscard.dll winscard.spec)

set_module_type(winscard win32dll)
target_link_libraries(winscard wine)
add_importlibs(winscard msvcrt kernel32 ntdll)
add_pch(winscard precomp.h SOURCE)
add_cd_file(TARGET winscard DESTINATION reactos/system32 FOR all)
