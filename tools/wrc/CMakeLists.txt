
list(APPEND SOURCE
    dumpres.c
    genres.c
    newstruc.c
    po.c
    readres.c
    translation.c
    utils.c
    wrc.c
    writeres.c
    parser.tab.c
    lex.yy.c
    port/mkstemps.c)

add_executable(wrc ${SOURCE})

target_link_libraries(wrc wpp unicode)
