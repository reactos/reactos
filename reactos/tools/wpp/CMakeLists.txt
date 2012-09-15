
if(MSVC)
    add_definitions(
        -Dsnprintf=_snprintf
        -Dstrtoull=_strtoui64
        -Dstrtoll=_strtoi64)

    # VS 9 doesn't like this
    if (NOT MSVC90)
        add_definitions(
            -Dvsnprintf=_vsnprintf)
    endif()
endif()

list(APPEND SOURCE
    lex.yy.c
    preproc.c
    wpp.c
    ppy.tab.c)

add_library(wpp ${SOURCE})

add_dependencies(wpp build_header)
