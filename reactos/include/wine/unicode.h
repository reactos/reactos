#ifndef _WINE_UNICODE_H
#define _WINE_UNICODE_H

#define strlenW(s) wcslen((const wchar_t *)s)
#define strcpyW(d,s) wcscpy((wchar_t *)d,(const wchar_t *)s)
#define strcatW(d,s) wcscat((wchar_t *)d,(const wchar_t *)s)
#define strstrW(d,s) wcsstr((const wchar_t *)d,(const wchar_t *)s)
#define strtolW(s,e,b) wcstol((const wchar_t *)s,(wchar_t **)e,b)
#define strchrW(s,c) wcschr((const wchar_t *)s,(wchar_t)c)
#define strrchrW(s,c) wcsrchr((const wchar_t *)s,(wchar_t)c)
#define strncmpW(s1,s2,n) wcsncmp((const wchar_t *)s1,(const wchar_t *)s2,n)
#define strncpyW(s1,s2,n) wcsncpy((wchar_t *)s1,(const wchar_t *)s2,n)
#define strcmpW(s1,s2) wcscmp((const wchar_t *)s1,(const wchar_t *)s2)
#define strcmpiW(s1,s2) _wcsicmp((const wchar_t *)s1,(const wchar_t *)s2)
#define strncmpiW(s1,s2,n) _wcsnicmp((const wchar_t *)s1,(const wchar_t *)s2,n)
#define tolowerW(n) towlower((wint_t)n)
#define toupperW(n) towupper((wint_t)n)
#define islowerW(n) iswlower((wint_t)n)
#define isupperW(n) iswupper((wint_t)n)
#define isalphaW(n) iswalpha((wint_t)n)
#define isalnumW(n) iswalnum((wint_t)n)
#define isdigitW(n) iswdigit((wint_t)n)
#define isxdigitW(n) iswxdigit((wint_t)n)
#define isspaceW(n) iswspace((wint_t)n)
#define atoiW(s) _wtoi((const wchar_t *)s)
#define atolW(s) _wtol((const wchar_t *)s)

#endif
