
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/dmilib)

list(APPEND SOURCE
    udmihelp.c
    precomp.h)

add_library(udmihelp ${SOURCE})
target_link_libraries(udmihelp dmilib)
add_pch(udmihelp precomp.h SOURCE)
add_dependencies(udmihelp psdk)
