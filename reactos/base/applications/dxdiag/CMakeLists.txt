
set_unicode()
set_rc_compiler()

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

add_executable(dxdiag
    ${CMAKE_CURRENT_BINARY_DIR}/dxdiag_precomp.h.gch
    ${SOURCE})

add_pch(dxdiag ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(dxdiag win32gui)

target_link_libraries(dxdiag dxguid)

add_importlibs(dxdiag user32 advapi32 comctl32 dinput8 setupapi dsound ddraw version gdi32 winmm d3d9 msvcrt kernel32 ntdll)

add_cd_file(TARGET dxdiag DESTINATION reactos/system32 FOR all)
