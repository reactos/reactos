include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/soft386)

list(APPEND SOURCE
    soft386.c)

add_library(soft386 ${SOURCE})
