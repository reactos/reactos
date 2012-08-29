
if(MSVC)
    add_definitions(
        -Dsnprintf=_snprintf
        -Dstrtoull=_strtoui64
        -Dstrtoll=_strtoi64
        -Dvsnprintf=_vsnprintf)
endif()

list(APPEND SOURCE
    lex.yy.c
    preproc.c
    wpp.c
    ppy.tab.c)

add_library(wpp ${SOURCE})

add_dependencies(wpp build_header)