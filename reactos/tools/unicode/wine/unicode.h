/*
 * Wine internal Unicode definitions
 *
 * Copyright 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_UNICODE_H
#define __WINE_UNICODE_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>

#ifndef WINE_UNICODE_API
#define WINE_UNICODE_API DECLSPEC_IMPORT
#endif

/* code page info common to SBCS and DBCS */
struct cp_info
{
    unsigned int          codepage;          /* codepage id */
    unsigned int          char_size;         /* char size (1 or 2 bytes) */
    WCHAR                 def_char;          /* default char value (can be double-byte) */
    WCHAR                 def_unicode_char;  /* default Unicode char value */
    const char           *name;              /* code page name */
};

struct sbcs_table
{
    struct cp_info        info;
    const WCHAR          *cp2uni;            /* code page -> Unicode map */
    const unsigned char  *uni2cp_low;        /* Unicode -> code page map */
    const unsigned short *uni2cp_high;
};

struct dbcs_table
{
    struct cp_info        info;
    const WCHAR          *cp2uni;            /* code page -> Unicode map */
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

extern const union cptable *wine_cp_get_table( unsigned int codepage );
extern const union cptable *wine_cp_enum_table( unsigned int index );

extern int wine_cp_mbstowcs( const union cptable *table, int flags,
                             const char *src, int srclen,
                             WCHAR *dst, int dstlen );
extern int wine_cp_wcstombs( const union cptable *table, int flags,
                             const WCHAR *src, int srclen,
                             char *dst, int dstlen, const char *defchar, int *used );
extern int wine_cpsymbol_mbstowcs( const char *src, int srclen, WCHAR *dst, int dstlen );
extern int wine_cpsymbol_wcstombs( const WCHAR *src, int srclen, char *dst, int dstlen );
extern int wine_utf8_mbstowcs( int flags, const char *src, int srclen, WCHAR *dst, int dstlen );
extern int wine_utf8_wcstombs( const WCHAR *src, int srclen, char *dst, int dstlen );

extern int wine_compare_string( int flags, const WCHAR *str1, int len1, const WCHAR *str2, int len2 );
extern int wine_get_sortkey( int flags, const WCHAR *src, int srclen, char *dst, int dstlen );
extern int wine_fold_string( int flags, const WCHAR *src, int srclen , WCHAR *dst, int dstlen );

extern int strcmpiW( const WCHAR *str1, const WCHAR *str2 );
extern int strncmpiW( const WCHAR *str1, const WCHAR *str2, int n );
extern int memicmpW( const WCHAR *str1, const WCHAR *str2, int n );
extern WCHAR *strstrW( const WCHAR *str, const WCHAR *sub );
extern long int strtolW( const WCHAR *nptr, WCHAR **endptr, int base );
extern unsigned long int strtoulW( const WCHAR *nptr, WCHAR **endptr, int base );
extern int sprintfW( WCHAR *str, const WCHAR *format, ... );
extern int snprintfW( WCHAR *str, size_t len, const WCHAR *format, ... );
extern int vsprintfW( WCHAR *str, const WCHAR *format, va_list valist );
extern int vsnprintfW( WCHAR *str, size_t len, const WCHAR *format, va_list valist );

extern inline int wine_is_dbcs_leadbyte( const union cptable *table, unsigned char ch )
{
    return (table->info.char_size == 2) && (table->dbcs.cp2uni_leadbytes[ch]);
}

extern inline WCHAR tolowerW( WCHAR ch )
{
    extern WINE_UNICODE_API const WCHAR wine_casemap_lower[];
    return ch + wine_casemap_lower[wine_casemap_lower[ch >> 8] + (ch & 0xff)];
}

extern inline WCHAR toupperW( WCHAR ch )
{
    extern WINE_UNICODE_API const WCHAR wine_casemap_upper[];
    return ch + wine_casemap_upper[wine_casemap_upper[ch >> 8] + (ch & 0xff)];
}

/* the character type contains the C1_* flags in the low 12 bits */
/* and the C2_* type in the high 4 bits */
extern inline unsigned short get_char_typeW( WCHAR ch )
{
    extern WINE_UNICODE_API const unsigned short wine_wctype_table[];
    return wine_wctype_table[wine_wctype_table[ch >> 8] + (ch & 0xff)];
}

extern inline int iscntrlW( WCHAR wc )
{
    return get_char_typeW(wc) & C1_CNTRL;
}

extern inline int ispunctW( WCHAR wc )
{
    return get_char_typeW(wc) & C1_PUNCT;
}

extern inline int isspaceW( WCHAR wc )
{
    return get_char_typeW(wc) & C1_SPACE;
}

extern inline int isdigitW( WCHAR wc )
{
    return get_char_typeW(wc) & C1_DIGIT;
}

extern inline int isxdigitW( WCHAR wc )
{
    return get_char_typeW(wc) & C1_XDIGIT;
}

extern inline int islowerW( WCHAR wc )
{
    return get_char_typeW(wc) & C1_LOWER;
}

extern inline int isupperW( WCHAR wc )
{
    return get_char_typeW(wc) & C1_UPPER;
}

extern inline int isalnumW( WCHAR wc )
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_DIGIT|C1_LOWER|C1_UPPER);
}

extern inline int isalphaW( WCHAR wc )
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_LOWER|C1_UPPER);
}

extern inline int isgraphW( WCHAR wc )
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER);
}

extern inline int isprintW( WCHAR wc )
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_BLANK|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/* some useful string manipulation routines */

extern inline unsigned int strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

extern inline WCHAR *strcpyW( WCHAR *dst, const WCHAR *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}

/* strncpy doesn't do what you think, don't use it */
#define strncpyW(d,s,n) error do_not_use_strncpyW_use_lstrcpynW_or_memcpy_instead

extern inline int strcmpW( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

extern inline int strncmpW( const WCHAR *str1, const WCHAR *str2, int n )
{
    if (n <= 0) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

extern inline WCHAR *strcatW( WCHAR *dst, const WCHAR *src )
{
    strcpyW( dst + strlenW(dst), src );
    return dst;
}

extern inline WCHAR *strchrW( const WCHAR *str, WCHAR ch )
{
    do { if (*str == ch) return (WCHAR *)str; } while (*str++);
    return NULL;
}

extern inline WCHAR *strrchrW( const WCHAR *str, WCHAR ch )
{
    WCHAR *ret = NULL;
    do { if (*str == ch) ret = (WCHAR *)str; } while (*str++);
    return ret;
}

extern inline WCHAR *strpbrkW( const WCHAR *str, const WCHAR *accept )
{
    for ( ; *str; str++) if (strchrW( accept, *str )) return (WCHAR *)str;
    return NULL;
}

extern inline size_t strspnW( const WCHAR *str, const WCHAR *accept )
{
    const WCHAR *ptr;
    for (ptr = str; *ptr; ptr++) if (!strchrW( accept, *ptr )) break;
    return ptr - str;
}

extern inline size_t strcspnW( const WCHAR *str, const WCHAR *reject )
{
    const WCHAR *ptr;
    for (ptr = str; *ptr; ptr++) if (strchrW( reject, *ptr )) break;
    return ptr - str;
}

extern inline WCHAR *strlwrW( WCHAR *str )
{
    WCHAR *ret = str;
    while ((*str = tolowerW(*str))) str++;
    return ret;
}

extern inline WCHAR *struprW( WCHAR *str )
{
    WCHAR *ret = str;
    while ((*str = toupperW(*str))) str++;
    return ret;
}

extern inline WCHAR *memchrW( const WCHAR *ptr, WCHAR ch, size_t n )
{
    const WCHAR *end;
    for (end = ptr + n; ptr < end; ptr++) if (*ptr == ch) return (WCHAR *)ptr;
    return NULL;
}

extern inline WCHAR *memrchrW( const WCHAR *ptr, WCHAR ch, size_t n )
{
    const WCHAR *end, *ret = NULL;
    for (end = ptr + n; ptr < end; ptr++) if (*ptr == ch) ret = ptr;
    return (WCHAR *)ret;
}

extern inline long int atolW( const WCHAR *str )
{
    return strtolW( str, (WCHAR **)0, 10 );
}

extern inline int atoiW( const WCHAR *str )
{
    return (int)atolW( str );
}

#endif  /* __WINE_UNICODE_H */
