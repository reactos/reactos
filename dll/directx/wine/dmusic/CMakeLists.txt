
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(dmusic.dll dmusic.spec)

list(APPEND SOURCE
    band.c
    buffer.c
    clock.c
    collection.c
    dmobject.c
    dmusic.c
    dmusic_main.c
    download.c
    instrument.c
    port.c
    wave.c
    precomp.h)

add_library(dmusic MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmusic.def)

add_idl_reg_scripts(dmusic registry dmusic.idl)

set_module_type(dmusic win32dll)
target_link_libraries(dmusic wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dmusic ole32 advapi32 winmm dsound user32 ucrtbase msvcrt kernel32 ntdll)
add_dependencies(dmusic wineheaders)
add_pch(dmusic precomp.h SOURCE)
add_cd_file(TARGET dmusic DESTINATION reactos/system32 FOR all)
set_wine_module(dmusic)
