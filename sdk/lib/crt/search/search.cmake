
list(APPEND LIBCNTPR_SEARCH_SOURCE
    search/bsearch.c
    search/lfind.c
)

list(APPEND CRT_SEARCH_SOURCE
    ${LIBCNTPR_SEARCH_SOURCE}
    search/lsearch.c
)
