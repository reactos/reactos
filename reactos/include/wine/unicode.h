#ifndef _WINE_UNICODE_H
#define _WINE_UNICODE_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>

/* code page info common to SBCS and DBCS */
struct cp_info
{
    unsigned int          codepage;          /* codepage id */
    unsigned int          char_size;         /* char size (1 or 2 bytes) */
    wchar_t               def_char;          /* default char value (can be double-byte) */
    wchar_t               def_unicode_char;  /* default Unicode char value */
    const char           *name;              /* code page name */
};

struct sbcs_table
{
    struct cp_info        info;
    const wchar_t        *cp2uni;            /* code page -> Unicode map */
    const unsigned char  *uni2cp_low;        /* Unicode -> code page map */
    const unsigned short *uni2cp_high;
};

struct dbcs_table
{
    struct cp_info        info;
    const wchar_t        *cp2uni;            /* code page -> Unicode map */
    const unsigned char  *cp2uni_leadbytes;
    const unsigned short *uni2cp_low;        /* Unicode -> code page map */
    const unsigned short *uni2cp_high;
    unsigned char         lead_bytes[12];    /* lead bytes ranges */
};

union cptable
{
    struct cp_info    info;
    struct sbcs_table sbcs;
    struct dbcs_table dbcs;
};

#define strlenW(s) wcslen((const wchar_t *)(s))
#define strcpyW(d,s) wcscpy((wchar_t *)(d),(const wchar_t *)(s))
#define strcatW(d,s) wcscat((wchar_t *)(d),(const wchar_t *)(s))
#define strstrW(d,s) wcsstr((const wchar_t *)(d),(const wchar_t *)(s))
#define strtolW(s,e,b) wcstol((const wchar_t *)(s),(wchar_t **)(e),(b))
#define strchrW(s,c) wcschr((const wchar_t *)(s),(wchar_t)(c))
#define strrchrW(s,c) wcsrchr((const wchar_t *)(s),(wchar_t)(c))
#define strncmpW(s1,s2,n) wcsncmp((const wchar_t *)(s1),(const wchar_t *)(s2),(n))
#define strncpyW(s1,s2,n) wcsncpy((wchar_t *)(s1),(const wchar_t *)(s2),(n))
#define strcmpW(s1,s2) wcscmp((const wchar_t *)(s1),(const wchar_t *)(s2))
#define strcmpiW(s1,s2) _wcsicmp((const wchar_t *)(s1),(const wchar_t *)(s2))
#define strncmpiW(s1,s2,n) _wcsnicmp((const wchar_t *)(s1),(const wchar_t *)(s2),(n))
#define tolowerW(n) towlower((n))
#define toupperW(n) towupper((n))
#define islowerW(n) iswlower((n))
#define isupperW(n) iswupper((n))
#define isalphaW(n) iswalpha((n))
#define isalnumW(n) iswalnum((n))
#define isdigitW(n) iswdigit((n))
#define isxdigitW(n) iswxdigit((n))
#define isspaceW(n) iswspace((n))
#define atoiW(s) _wtoi((const wchar_t *)(s))
#define atolW(s) _wtol((const wchar_t *)(s))
#define strlwrW(s) _wcslwr((wchar_t *)(s))
#define struprW(s) _wcsupr((wchar_t *)(s))
#define sprintfW wsprintfW
#define snprintfW _snwprintf

#ifndef WINE_UNICODE_API
#define WINE_UNICODE_API __attribute__((dllimport))
#endif

/* the character type contains the C1_* flags in the low 12 bits */
/* and the C2_* type in the high 4 bits */
static inline unsigned short get_char_typeW( wchar_t ch )
{
    extern WINE_UNICODE_API const unsigned short wine_wctype_table[];
    return wine_wctype_table[wine_wctype_table[ch >> 8] + (ch & 0xff)];
}

static inline WCHAR *strpbrkW( const WCHAR *str, const WCHAR *accept )
{
    for ( ; *str; str++) if (strchrW( accept, *str )) return (WCHAR *)str;
    return NULL;
}

#endif
