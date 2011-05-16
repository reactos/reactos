
list(APPEND SOURCE
    lex.yy.c
    preproc.c
    wpp.c
    ppy.tab.c)

add_library(wpp ${SOURCE})

add_dependencies(wpp build_header)