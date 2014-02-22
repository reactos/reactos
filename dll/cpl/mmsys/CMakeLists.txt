
spec2def(mmsys.cpl mmsys.spec)

list(APPEND SOURCE
    mmsys.c
    sounds.c
    volume.c
    audio.c
    voice.c
    mmsys.h)

add_library(mmsys SHARED
    ${SOURCE}
    mmsys.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mmsys.def)

set_module_type(mmsys cpl UNICODE)

add_importlibs(mmsys
    msvcrt
    user32
    comctl32
    devmgr
    gdi32
    winmm
    advapi32
    shell32
    setupapi
    shlwapi
    ntdll
    kernel32
    comdlg32)

add_pch(mmsys mmsys.h SOURCE)
add_cd_file(TARGET mmsys DESTINATION reactos/system32 FOR all)
