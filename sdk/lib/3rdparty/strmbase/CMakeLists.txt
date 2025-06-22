
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600 -D__WINESRC__)

list(APPEND SOURCE
    dispatch.c
    filter.c
    mediatype.c
    pin.c
    pospass.c
    renderer.c
    seeking.c
    precomp.h)

add_library(strmbase ${SOURCE})
add_pch(strmbase precomp.h SOURCE)
add_dependencies(strmbase psdk dxsdk)
