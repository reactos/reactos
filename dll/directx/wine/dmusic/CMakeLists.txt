
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(dmusic.dll dmusic.spec)

list(APPEND SOURCE
    buffer.c
    clock.c
    collection.c
    dmobject.c
    dmusic.c
    dmusic_main.c
    download.c
    instrument.c
    port.c
    precomp.h)

add_library(dmusic MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmusic.def)

set_module_type(dmusic win32dll)
target_link_libraries(dmusic dxguid uuid wine)
add_importlibs(dmusic ole32 advapi32 winmm dsound user32 msvcrt kernel32 ntdll)
add_pch(dmusic precomp.h SOURCE)
add_cd_file(TARGET dmusic DESTINATION reactos/system32 FOR all)
