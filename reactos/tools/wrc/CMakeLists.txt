
list(APPEND SOURCE
    dumpres.c
    genres.c
    newstruc.c
    readres.c
    translation.c
    utils.c
    wrc.c
    writeres.c
    parser.tab.c
    lex.yy.c
    port/mkstemps.c)

add_executable(wrc ${SOURCE})

add_definitions(-DWINE_UNICODE_API= )

target_link_libraries(wrc wpp unicode)
