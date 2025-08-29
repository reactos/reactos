
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600 -D__WINESRC__)

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
    precomp.h)

add_library(strmbase ${SOURCE})

if(MSVC AND ARCH STREQUAL "amd64")
    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
    target_compile_options(strmbase PRIVATE "/wd4267")
endif()

add_pch(strmbase precomp.h SOURCE)
add_dependencies(strmbase psdk dxsdk)
