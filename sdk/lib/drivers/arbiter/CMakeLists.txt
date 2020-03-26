
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/rtl)

list(APPEND SOURCE
    arbiter.c
    arbiter.h)

add_library(arbiter ${SOURCE})
add_dependencies(arbiter bugcodes xdk)
add_pch(arbiter arbiter.h SOURCE)
