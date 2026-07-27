
# performance.c drives its message pump with a CONDITION_VARIABLE.
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x600
    -DWINVER=0x600)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dmusic)

spec2def(dmime.dll dmime.spec)

list(APPEND SOURCE
    audiopath.c
    dmime_main.c
    graph.c
    lyricstrack.c
    markertrack.c
    midi.c
    paramcontroltrack.c
    performance.c
    segment.c
    segmentstate.c
    segtriggertrack.c
    seqtrack.c
    sysextrack.c
    tempotrack.c
    timesigtrack.c
    wavetrack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/band.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/dmobject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/wave.c)

add_library(dmime MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmime.def)

add_idl_reg_scripts(dmime registry dmime.idl)

set_module_type(dmime win32dll)
target_link_libraries(dmime wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dmime dsound ole32 user32 advapi32 kernel32_vista ucrtbase msvcrt kernel32 ntdll)
add_dependencies(dmime wineheaders)
add_cd_file(TARGET dmime DESTINATION reactos/system32 FOR all)
set_wine_module(dmime)
