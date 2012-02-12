
add_definitions(
    -DUNICODE -D_UNICODE
    -DNDEBUG)

list(APPEND SOURCE
    context.c
    hidparser.c
    parser.c
    api.c)

add_library(hidparser ${SOURCE})
add_dependencies(hidparser bugcodes)

