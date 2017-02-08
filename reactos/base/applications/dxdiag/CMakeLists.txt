
list(APPEND SOURCE
    system.c
    display.c
    sound.c
    music.c
    input.c
    network.c
    help.c
    dxdiag.c
    dxdiag.rc
    ddtest.c
    d3dtest.c
    d3dtest7.c
    d3dtest8.c
    d3dtest9.c)

add_executable(dxdiag ${SOURCE})
set_module_type(dxdiag win32gui UNICODE)
target_link_libraries(dxdiag dxguid)
add_importlibs(dxdiag user32 advapi32 comctl32 dinput8 setupapi dsound ddraw version gdi32 winmm d3d9 msvcrt kernel32 ntdll)
add_pch(dxdiag precomp.h)
add_cd_file(TARGET dxdiag DESTINATION reactos/system32 FOR all)
