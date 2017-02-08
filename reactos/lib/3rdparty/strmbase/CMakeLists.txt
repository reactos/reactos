
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    audio.c
    dispatch.c
    dllfunc.c
    enumpins.c
    filter.c
    mediatype.c
    outputqueue.c
    pin.c
    pospass.c
    qualitycontrol.c
    renderer.c
    seeking.c
    transform.c
    video.c
    window.c
    strmbase_private.h)

add_library(strmbase ${SOURCE})
add_pch(strmbase strmbase_private.h SOURCE)
add_dependencies(strmbase psdk dxsdk)
