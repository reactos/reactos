
set_rc_compiler()

spec2def(mmsys.cpl mmsys.spec)

add_library(mmsys SHARED
    mmsys.c
    sounds.c
    volume.c
    audio.c
    voice.c
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
    kernel32)

add_pch(mmsys mmsys.h)
add_cd_file(TARGET mmsys DESTINATION reactos/system32 FOR all)
