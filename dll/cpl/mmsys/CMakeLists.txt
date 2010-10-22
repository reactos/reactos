
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(mmsys ${CMAKE_CURRENT_SOURCE_DIR}/mmsys.spec ${CMAKE_CURRENT_BINARY_DIR}/mmsys.def)

add_library(mmsys SHARED
    mmsys.c
    sounds.c
    volume.c
    audio.c
    voice.c
    mmsys.rc)

set_module_type(mmsys cpl)

target_link_libraries(mmsys ${CMAKE_CURRENT_BINARY_DIR}/mmsys.def)

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

add_dependencies(mmsys mmsys_def)
add_livecd_target(mmsys reactos/system32)
