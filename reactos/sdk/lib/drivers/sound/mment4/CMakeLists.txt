
add_definitions(
    -DUNICODE -D_UNICODE
    -DDEBUG_NT4=1)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/sound)

list(APPEND SOURCE
    detect.c
    registry.c
    control.c
    precomp.h)

add_library(mment4 ${SOURCE})
add_pch(mment4 precomp.h SOURCE)
add_dependencies(mment4 psdk)
