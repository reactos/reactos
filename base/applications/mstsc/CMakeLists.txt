
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

add_executable(mstsc ${SOURCE})

set_module_type(mstsc win32gui)

add_importlibs(mstsc user32 gdi32 comctl32 ws2_32 advapi32 shell32 ole32 comdlg32 msvcrt kernel32)

add_pch(mstsc precomp.h)

add_cd_file(TARGET mstsc DESTINATION reactos/system32 FOR all)
