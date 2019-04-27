
add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    context.c
    hidparser.c
    parser.c
    api.c
    parser.h
    hidp.c
    hidp.h)

add_library(hidparser_km ${SOURCE})
add_importlibs(hidparser_km ntoskrnl)
add_dependencies(hidparser_km bugcodes xdk)
add_pch(hidparser_km parser.h SOURCE)
