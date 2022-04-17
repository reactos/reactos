
list(APPEND LIBCNTPR_WSTRING_SOURCE
    wstring/wcsicmp.c
    wstring/wcslwr.c
    wstring/wcsnicmp.c
    wstring/wcsupr.c
    wstring/wcscspn.c
    wstring/wcsspn.c
    wstring/wcsstr.c
)

list(APPEND CRT_WSTRING_SOURCE
    ${LIBCNTPR_WSTRING_SOURCE}
    wstring/wcscoll.c
    wstring/wcstok.c
    wstring/wcsxfrm.c
)
