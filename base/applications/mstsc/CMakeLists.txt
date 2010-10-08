
set_unicode()

list(APPEND SOURCE
    bitmap.c
    bsops.c
    cache.c
    channels.c
    connectdialog.c
    iso.c
    licence.c
    mcs.c
    mppc.c
    orders.c
    pstcache.c
    rdp5.c
    rdp.c
    secure.c
    settings.c
    ssl_calls.c
    tcp.c
    uimain.c
    win32.c
    rdc.rc)

add_executable(mstsc ${CMAKE_CURRENT_BINARY_DIR}/mstsc_precomp.h.gch ${SOURCE})

add_pch(mstsc ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(mstsc win32gui)

add_importlibs(mstsc user32 gdi32 comctl32 ws2_32 advapi32 shell32 ole32 comdlg32 msvcrt kernel32)
