
add_definitions(
    -DUNICODE -D_UNICODE
    -DNDEBUG=1)

list(APPEND SOURCE
    hidparser.c
    parser.c
    api.c)

add_library(hidparser ${SOURCE})
add_dependencies(hidparser bugcodes)

