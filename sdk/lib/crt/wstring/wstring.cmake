
list(APPEND LIBCNTPR_WSTRING_SOURCE
    wstring/wcslwr.c
    wstring/wcsupr.c
    wstring/wcscspn.c
    wstring/wcsspn.c
    wstring/wcsstr.c
)

list(APPEND CRT_WSTRING_SOURCE
    ${LIBCNTPR_WSTRING_SOURCE}
    wstring/mbrtowc.c
    wstring/wcrtomb.c
    wstring/wcscoll.c
    wstring/wcsicmp.c
    wstring/wcsnicmp.c
    wstring/wcstok.c
    wstring/wcsxfrm.c
)

list(APPEND LIBCNTPR_WSTRING_SOURCE
    wstring/_wcsicmp_nt.c
    wstring/_wcsnicmp_nt.c
)
