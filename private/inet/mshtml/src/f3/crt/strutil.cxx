//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       strutil.cxx
//
//  Contents:   These string comparison functions replace the C runtime's
//              implementation. They use the correct locale for string
//              comparison and make it unnecessary to link to the C runtime.
//
//              Also contains our versions of the _is??? C-Runtime functions
//
//  Note:       For Far East compatibility, the case insensitive compare functions
//              should also ignore char width (single-byte 'a' is the same
//              as double-byte 'a') and kana type (this is Japanese-specific).
//  BUGBUG:     How do we convert the internal Unicode representation to BDCS?
//              Is it possible to convert in a way that we can avoid these extra
//              comparison attributes for performance's sake?
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CTYPE_H_
#define X_CTYPE_H_
#include <ctype.h>
#endif

#if defined(_MAC) && !defined(_MACUNICODE)
#define RETURN_TYPE int
#else
#define RETURN_TYPE int __cdecl
#endif

// IEUNIX BUGBUG: From NT's string.h  Where should this go?
#ifdef UNIX
#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR    2147483647  /* currently == INT_MAX */
#define _NLSCMP_DEFINED
#endif
#endif

#ifndef IsCharDigit
BOOL
IsCharDigit(TCHAR ch)
{
    return (unsigned)(ch - L'0') <= (L'9' - L'0');
}
#endif

BOOL
_tcsequal(const TCHAR *string1, const TCHAR *string2)
{
    // This function optimizes the case where all we want to find
    // is if the strings are equal and do not care about the relative
    // ordering of the strings.

    while (*string1)
    {
        if (*string1 != *string2)
            return FALSE;

        string1 += 1;
        string2 += 1;

    }
    return (*string2) ? (FALSE) : (TRUE);
}

BOOL
_tcsiequal(const TCHAR *string1, const TCHAR *string2)
{
    // This function optimizes the case where all we want to find
    // is if the strings are equal and do not care about the relative
    // ordering of the strings (or CaSe).

    while (*string1)
    {
        TCHAR ch1 = *string1;
        TCHAR ch2 = *string2;

        CharLowerBuff(&ch1, 1);
        CharLowerBuff(&ch2, 1);

        if (ch1 != ch2)
            return FALSE;

        string1 += 1;
        string2 += 1;

    }
    return (*string2) ? (FALSE) : (TRUE);
}

BOOL
_tcsnpre(const TCHAR * string1, int cch1, const TCHAR * string2, int cch2)
{
    if (cch1 == -1)
        cch1 = _tcslen(string1);
    if (cch2 == -1)
        cch2 = _tcslen(string2);
    return( cch1 <= cch2
        &&  CompareString(g_lcidUserDefault, 0, string1, cch1,
                string2, cch1) == 2);
}

BOOL
_tcsnipre(const TCHAR * string1, int cch1, const TCHAR * string2, int cch2)
{
    if (cch1 == -1)
        cch1 = _tcslen(string1);
    if (cch2 == -1)
        cch2 = _tcslen(string2);
    return( cch1 <= cch2
        &&  CompareString(g_lcidUserDefault,
                NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                string1, cch1, string2, cch1) == 2);
}

#ifndef WIN16
RETURN_TYPE
_tcscmp(const wchar_t *string1, const wchar_t *string2)
{
    int cc;

    cc = CompareString(g_lcidUserDefault, 0, string1, -1, string2, -1);

    if ( cc > 0 )
        return(cc - 2);     //  CompareString returns values in the 1..3 range
    else
        return _NLSCMPERROR;
}

RETURN_TYPE
_tcsicmp(const wchar_t *string1, const wchar_t *string2)
{
    int cc;

    cc = CompareString(g_lcidUserDefault,
                       NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                       string1, -1,
                       string2, -1);

    if ( cc > 0 )
        return(cc - 2);
    else
        return _NLSCMPERROR;
}

#ifndef UNIX
// Matches behavior of CRT's _wcsicmp -- case insensitive, lexographic (i.e. stringsort
// instead of wordsort), locale insensitive.
RETURN_TYPE
_wcsicmp(const wchar_t *string1, const wchar_t *string2)
{
    int cc;

    cc = CompareString(0,   // deliberately NOT locale sensitive
                       NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE | SORT_STRINGSORT,
                       string1, -1,
                       string2, -1);

    if ( cc > 0 )
        return(cc - 2);
    else
        return _NLSCMPERROR;
}
#endif

RETURN_TYPE
_tcsncmp(const wchar_t *string1, int cch1, const wchar_t *string2, int cch2)
{
    int cc;

    cc = CompareString(g_lcidUserDefault,
                       0,
                       string1, cch1,
                       string2, cch2);

    if ( cc > 0 )
        return(cc - 2);
    else
        return _NLSCMPERROR;
}

RETURN_TYPE
_tcsnicmp(const wchar_t *string1, int cch1, const wchar_t *string2, int cch2)
{
    int cc;

    cc = CompareString(g_lcidUserDefault,
                       NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                       string1, cch1,
                       string2, cch2);

    if ( cc > 0 )
        return(cc - 2);
    else
        return _NLSCMPERROR;
}



//
// BUGBUG -- would it be better to have these inline?
//

RETURN_TYPE
_istspace(TCHAR ch)
{
    return IsCharSpace(ch);
}

RETURN_TYPE
_istdigit(TCHAR ch)
{
    return IsCharDigit(ch);
}

RETURN_TYPE
_istxdigit(TCHAR ch)
{
    return IsCharXDigit(ch);
}

RETURN_TYPE
_istalpha(TCHAR ch)
{
    return IsCharAlpha(ch);
}

RETURN_TYPE
_istalnum(TCHAR ch)
{
    return IsCharAlphaNumeric(ch);
}

RETURN_TYPE
_istprint(TCHAR ch)
{
    return IsCharCntrl(ch) ? 0 : 1;
}

#ifndef _MAC

RETURN_TYPE
isspace(int ch)
{
    return IsCharSpace((USHORT)ch);
}

RETURN_TYPE
isdigit(int ch)
{
    WORD    charType;

    BOOL fRet = GetStringTypeExA(g_lcidUserDefault, CT_CTYPE1, (LPCSTR) &ch, 1, &charType);
    if (!fRet)
        return 0;

    return (charType & C1_DIGIT) ? 1: 0;
}

RETURN_TYPE
isalpha(int ch)
{
    WORD    charType;

    BOOL fRet = GetStringTypeExA(g_lcidUserDefault, CT_CTYPE1, (LPCSTR) &ch, 1, &charType);
    if (!fRet)
        return 0;

    return (charType & C1_ALPHA) ? 1: 0;
}

RETURN_TYPE
iswspace(wchar_t ch)
{
    return IsCharSpace(ch);
}
#endif // _MAC
#endif // !WIN16

//+------------------------------------------------------------------------
//
//  Member:     _7csicmp
//
//  Synopsis:   Compare two 7 bit ascii strings case insensitive
//
//-------------------------------------------------------------------------

int
_7csicmp(const TCHAR *pch1, const TCHAR *pch2)
{
    while (*pch1)
    {
        TCHAR ch1 = *pch1 >= _T('a') && *pch1 <= _T('z') ?
                *pch1 + _T('A') - _T('a') : *pch1;

        TCHAR ch2 = *pch2 >= _T('a') && *pch2 <= _T('z') ?
                *pch2 + _T('A') - _T('a') : *pch2;

        if (ch1 > ch2)
            return 1;
        else if (ch1 < ch2)
            return -1;

        pch1 += 1;
        pch2 += 1;
    }

    return *pch2 ? -1 : 0;
}


//+------------------------------------------------------------------------
//
//  Member:     _7cscmp
//
//  Synopsis:   Compare two 7 bit ascii strings case insensitive
//
//-------------------------------------------------------------------------

int
_7cscmp(const TCHAR *pch1, const TCHAR *pch2)
{
    while (*pch1)
    {
        TCHAR ch1 = *pch1;
        TCHAR ch2 = *pch2;

        if (ch1 > ch2)
            return 1;
        else if (ch1 < ch2)
            return -1;

        pch1 += 1;
        pch2 += 1;
    }

     return *pch2 ? -1 : 0;
}

BOOL
_7csnipre(const TCHAR * pch1, int cch1, const TCHAR * pch2, int cch2)
{
    if (cch1 == -1)
        cch1 = _tcslen(pch1);
    if (cch2 == -1)
        cch2 = _tcslen(pch2);
    if (cch1 <= cch2)
    {
        while (cch1)
        {
            TCHAR ch1 = *pch1 >= _T('a') && *pch1 <= _T('z') ?
                    *pch1 + _T('A') - _T('a') : *pch1;

            TCHAR ch2 = *pch2 >= _T('a') && *pch2 <= _T('z') ?
                    *pch2 + _T('A') - _T('a') : *pch2;

            if (ch1 != ch2)
                return FALSE;

            pch1 += 1;
            pch2 += 1;
            cch1--;
        }
        return TRUE;
    }
    else
        return FALSE;
}

#ifndef WIN16
wchar_t * __cdecl _wcsdup(const wchar_t *pcszOriginal)
{
    WCHAR *pszCopy = (WCHAR *)MemAlloc( Mt(Mem), (_tcslen( pcszOriginal ) + 1)*sizeof(WCHAR) );
    if ( pszCopy )
        _tcscpy( pszCopy, pcszOriginal );
    return pszCopy;
}
#endif // ndef WIN16


const TCHAR * __cdecl _tcsistr (const TCHAR * tcs1,const TCHAR * tcs2)
{
    const TCHAR *cp;
    int cc,count;
    int n2Len = _tcslen ( tcs2 );
    int n1Len = _tcslen ( tcs1 );

    if ( n1Len >= n2Len )
    {
        for ( cp = tcs1, count = n1Len - n2Len; count>=0 ; cp++,count-- )
        {
            cc = CompareString(g_lcidUserDefault,
                NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                cp, n2Len,tcs2, n2Len);
            if ( cc > 0 )
                cc-=2;
            if ( !cc )
                return cp;
        }
    }
    return NULL;
}
