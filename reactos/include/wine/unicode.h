#ifndef _WINE_UNICODE_H
#define _WINE_UNICODE_H

#define strlenW(s) wcslen((const wchar_t *)s)
#define strcpyW(d,s) wcscpy((wchar_t *)d,(const wchar_t *)s)
#define strcatW(d,s) wcscat((wchar_t *)d,(const wchar_t *)s)
#define strstrW(d,s) wcsstr((const wchar_t *)d,(const wchar_t *)s)
#define strtolW(s,e,b) wcstol((const wchar_t *)s,(wchar_t **)e,b)
#define strchrW(s,c) wcschr((const wchar_t *)s,(wchar_t)c)
#define strncmpW(s1,s2,n) wcsncmp((const wchar_t *)s1,(const wchar_t *)s2,n)
#define tolowerW(n) towlower((wint_t)n)
#define atoiW(s) _wtoi((const wchar_t *)s)
#define atolW(s) _wtol((const wchar_t *)s)

#endif
