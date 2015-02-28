
list(APPEND SOURCE
    asn.c
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
    bsops.h
    constants.h
    orders.h
    proto.h
    rdesktop.h
    resource.h
    types.h
    uimain.h
    precomp.h)

add_executable(mstsc ${SOURCE} rdc.rc)
set_module_type(mstsc win32gui UNICODE)
add_importlibs(mstsc user32 gdi32 comctl32 ws2_32 advapi32 shell32 ole32 comdlg32 msvcrt kernel32)
add_pch(mstsc precomp.h SOURCE)
add_cd_file(TARGET mstsc DESTINATION reactos/system32 FOR all)
