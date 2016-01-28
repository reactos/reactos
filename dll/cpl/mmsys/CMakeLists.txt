
spec2def(mmsys.cpl mmsys.spec)

list(APPEND SOURCE
    mmsys.c
    sounds.c
    volume.c
    audio.c
    voice.c
    mmsys.h)

file(GLOB mmsys_rc_deps resources/*.*)
add_rc_deps(mmsys.rc ${mmsys_rc_deps})

add_library(mmsys SHARED
    ${SOURCE}
    mmsys.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mmsys.def)

set_module_type(mmsys cpl UNICODE)
add_importlibs(mmsys user32 comctl32 devmgr gdi32 winmm advapi32 shell32 setupapi shlwapi comdlg32 msvcrt kernel32 ntdll)
add_pch(mmsys mmsys.h SOURCE)
add_cd_file(TARGET mmsys DESTINATION reactos/system32 FOR all)
