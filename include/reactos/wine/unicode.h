/*
 * PROJECT:    ReactOS
 * LICENSE:    LGPL v2.1 or any later version
 * PURPOSE:    Map Wine's Unicode functions to native wcs* functions wherever possible
 * AUTHORS:    ?
 */

#ifndef __WINE_WINE_UNICODE_H
#define __WINE_WINE_UNICODE_H

#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>

#ifndef WINE_UNICODE_API
#define WINE_UNICODE_API
#endif

#ifndef WINE_UNICODE_INLINE
#define WINE_UNICODE_INLINE static inline
#endif

#define memicmpW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strlenW(s) wcslen((s))
#define strcpyW(d,s) wcscpy((d),(s))
#define strcatW(d,s) wcscat((d),(s))
#define strcspnW(d,s) wcscspn((d),(s))
#define strstrW(d,s) wcsstr((d),(s))
#define strtolW(s,e,b) wcstol((s),(e),(b))
#define strchrW(s,c) wcschr((s),(c))
#define strrchrW(s,c) wcsrchr((s),(c))
#define strncmpW(s1,s2,n) wcsncmp((s1),(s2),(n))
#define strncpyW(s1,s2,n) wcsncpy((s1),(s2),(n))
#define strcmpW(s1,s2) wcscmp((s1),(s2))
#define strcmpiW(s1,s2) _wcsicmp((s1),(s2))
#define strncmpiW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strtoulW(s1,s2,b) wcstoul((s1),(s2),(b))
#define strspnW(str, accept) wcsspn((str),(accept))
#define strpbrkW(str, accept) wcspbrk((str),(accept))
#define tolowerW(n) towlower((n))
#define toupperW(n) towupper((n))
#define islowerW(n) iswlower((n))
#define isupperW(n) iswupper((n))
#define isalphaW(n) iswalpha((n))
#define isalnumW(n) iswalnum((n))
#define isdigitW(n) iswdigit((n))
#define isxdigitW(n) iswxdigit((n))
#define isspaceW(n) iswspace((n))
#define iscntrlW(n) iswcntrl((n))
#define atoiW(s) _wtoi((s))
#define atolW(s) _wtol((s))
#define strlwrW(s) _wcslwr((s))
#define struprW(s) _wcsupr((s))
#define sprintfW swprintf
#define vsprintfW vswprintf
#define snprintfW _snwprintf
#define vsnprintfW _vsnwprintf
#define isprintW iswprint

static __inline unsigned short get_char_typeW( WCHAR ch )
{
    extern const unsigned short wine_wctype_table[];
    return wine_wctype_table[wine_wctype_table[ch >> 8] + (ch & 0xff)];
}

static __inline WCHAR *memchrW( const WCHAR *ptr, WCHAR ch, size_t n )
{
    const WCHAR *end;
    for (end = ptr + n; ptr < end; ptr++) if (*ptr == ch) return (WCHAR *)ptr;
    return NULL;
}

static __inline WCHAR *memrchrW( const WCHAR *ptr, WCHAR ch, size_t n )
{
    const WCHAR *end, *ret = NULL;
    for (end = ptr + n; ptr < end; ptr++) if (*ptr == ch) ret = ptr;
    return (WCHAR *)ret;
}

#endif
