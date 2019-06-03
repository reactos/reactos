
list(APPEND SOURCE
    entry.c
    mme.c
    kernel.c
    session.c
    common.c
    wave.c
    wave_io.c
    mmdrv.h)

add_library(mmdrv MODULE ${SOURCE})
spec2def(mmdrv mmdrv.dll mmdrv.spec)

set_module_type(mmdrv win32dll UNICODE)
add_importlibs(mmdrv winmm user32 msvcrt kernel32 ntdll)
add_pch(mmdrv mmdrv.h SOURCE)
add_cd_file(TARGET mmdrv DESTINATION reactos/system32 FOR all)
