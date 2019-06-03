
add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    mciwnd.c
    msvideo_main.c
    drawdib.c
    precomp.h)

add_library(msvfw32 MODULE
    ${SOURCE}
    rsrc.rc)
spec2def(msvfw32 msvfw32.dll msvfw32.spec ADD_IMPORTLIB)

set_module_type(msvfw32 win32dll)
target_link_libraries(msvfw32 wine)
add_importlibs(msvfw32 winmm comctl32 user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_pch(msvfw32 precomp.h SOURCE)
add_cd_file(TARGET msvfw32 DESTINATION reactos/system32 FOR all)
