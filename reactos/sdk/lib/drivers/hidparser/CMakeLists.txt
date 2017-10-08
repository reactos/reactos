
add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    context.c
    hidparser.c
    parser.c
    api.c
    parser.h)

add_library(hidparser ${SOURCE})
add_dependencies(hidparser bugcodes xdk)
add_pch(hidparser parser.h SOURCE)
