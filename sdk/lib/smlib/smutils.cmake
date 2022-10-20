
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

list(APPEND SOURCE
    smutils.c
    precomp.h)

add_library(smutils ${SOURCE})
target_link_libraries(smutils smlib)
add_pch(smutils precomp.h SOURCE)
add_dependencies(smutils psdk)
