
add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    context.c
    hidparser.c
    parser.c
    api.c)

add_library(hidparser ${SOURCE})
add_dependencies(hidparser bugcodes)

