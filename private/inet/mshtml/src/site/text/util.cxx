/*
 *  UTIL.C
 *
 *  Purpose:
 *      Implementation of various useful utility functions
 *
 *  Author:
 *      alexgo (4/25/95)
 */

#include "headers.hxx"

#ifndef WIN16
#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx" // for cstrinw
#endif
#endif

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

// BUGBUG (cthrash) The following table needs to reflect the new order of the
// funfork.
// This is used by textedit.h

/*
 *  DuplicateHGlobal
 *
 *  Purpose:
 *      duplicates the passed in hglobal
 */

HGLOBAL DuplicateHGlobal( HGLOBAL hglobal )
{
    UINT    flags;
    DWORD   size;
    HGLOBAL hNew;
    BYTE *  pSrc;
    BYTE *  pDest;

    if( hglobal == NULL )
    {
        return NULL;
    }

    flags = GlobalFlags(hglobal);

    size = GlobalSize(hglobal);

    hNew = GlobalAlloc(flags, size);

    if( hNew )
    {
        pDest = (BYTE *)GlobalLock(hNew);
        pSrc = (BYTE *)GlobalLock(hglobal);

        if( pDest == NULL || pSrc == NULL )
        {
            GlobalUnlock(hNew);
            GlobalUnlock(hglobal);
            GlobalFree(hNew);

            return NULL;
        }

        memcpy(pDest, pSrc, size);

        GlobalUnlock(hNew);
        GlobalUnlock(hglobal);
    }

    return hNew;
}

#ifdef WIN16
HGLOBAL TextHGlobalAtoW( HGLOBAL hglobalA )
{
    return DuplicateHGlobal( hglobalA );
}

HGLOBAL TextHGlobalWtoA( HGLOBAL hglobalW )
{
    return DuplicateHGlobal( hglobalW );
}

#else
/*
 *  TextHGlobalAtoW (hglobalA)
 *
 *  Purpose:
 *      translates a unicode string contained in an hglobal and
 *      wraps the ansi version in another hglobal
 *
 *  Notes: 
 *      does *not* free the incoming hglobal
 */

HGLOBAL TextHGlobalAtoW( HGLOBAL hglobalA )
{
    LPSTR   pstr;
    HGLOBAL hnew = NULL;
    DWORD   cbSize;

    if( !hglobalA )
    {
        return NULL;
    }

    pstr = (LPSTR)GlobalLock(hglobalA);

    CStrInW  strinw(pstr);

    cbSize = (strinw.strlen() + 1) * sizeof(WCHAR);
    hnew = GlobalAlloc(GMEM_MOVEABLE, cbSize);

    if( hnew )
    {
        LPWSTR pwstr = (LPWSTR)GlobalLock(hnew);
        
        if( pwstr )
        {
            memcpy(pwstr, (WCHAR *)strinw, cbSize);
        
            GlobalUnlock(hnew);
        }
    }

    GlobalUnlock(hglobalA);
    
    return hnew;
}

/*
 *  TextHGlobalAtoW
 *
 *  Purpose:
 *      converts a unicode text hglobal into a newly allocated
 *      allocated hglobal with ANSI data
 *
 *  Notes:
 *      does *NOT* free the incoming hglobal 
 */
    
HGLOBAL TextHGlobalWtoA( HGLOBAL hglobalW )
{
    LPCWSTR  pwstr;
    HGLOBAL hnew = NULL;
    DWORD   cbSize;

    if( !hglobalW )
    {
        return NULL;
    }

    pwstr = (LPCWSTR)GlobalLock(hglobalW);

    CStrIn  strin(pwstr);

    cbSize = (strin.strlen() + 1) * sizeof(CHAR);
    hnew = GlobalAlloc(GMEM_MOVEABLE, cbSize);

    if( hnew )
    {
        LPSTR pstr = (LPSTR)GlobalLock(hnew);
        
        if( pstr )
        {
            memcpy(pstr, (CHAR *)strin, cbSize);
        
            GlobalUnlock(hnew);
        }
    }

    GlobalUnlock(hglobalW);
    
    return hnew;
}   
#endif // !WIN16

/*
 *  CountMatchingBits ( const DWORD *a, const DWORD *b, INT total )
 *
 *  @mfunc
 *      Count matching bit fields.
 *  @comm
 *      This is used to help decide how good the match is between
 *      code page bit fields. Mainly for KB/font switching support.
 *  Author:
 *      Jon Matousek
 */
INT CountMatchingBits(const DWORD *a, const DWORD *b, INT total)
{
    static INT  bitCount[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
    INT         i, c, matchBits;

    c = 0;
    for (i = 0; i < total; i++ )
    {
        //matchBits = ~(*a++ ^ *b++);   // 1 and 0's
        matchBits = *a++ & *b++;        // 1 only.
        c += bitCount [ (matchBits >> 0)    & 15];
        c += bitCount [ (matchBits >> 4)    & 15];
        c += bitCount [ (matchBits >> 8)    & 15];
        c += bitCount [ (matchBits >> 12)   & 15];
        c += bitCount [ (matchBits >> 16)   & 15];
        c += bitCount [ (matchBits >> 20)   & 15];
        c += bitCount [ (matchBits >> 24)   & 15];
        c += bitCount [ (matchBits >> 28)   & 15];
    }

    return c;
}

//+----------------------------------------------------------------------------
//
//  Function:   HtmlStringToHGlobal
//
//  Synopsis:   Build an HGLOBAL from a unicode html string.  Specifivally, it
//              places the correct signature at the beginning of the HGLOBAL so
//              paste code knows what the hell it is.
//
//-----------------------------------------------------------------------------

HRESULT
HtmlStringToSignaturedHGlobal (
    HGLOBAL * phglobal, const TCHAR * pStr, long cch )
{
    HRESULT hr = S_OK;
    char *  pStrHtmlText = NULL;
    long    cbStr;
    long    lGlobalSize;

    Assert( cch >= -1 );
    Assert( phglobal );

    //
    // Allocate enough for the string, unicode signature and zero terminator
    //
    
    cbStr = (cch < 0 ? _tcslen( pStr ) : cch) * sizeof( TCHAR );
    
    lGlobalSize = cbStr + sizeof( WCHAR ) + sizeof( TCHAR );

    *phglobal = GlobalAlloc( GMEM_MOVEABLE, lGlobalSize );

    if (!*phglobal)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pStrHtmlText = (char *) GlobalLock( *phglobal );

    if (!pStrHtmlText)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    //
    // Put the unicaode signature at the beginning, followed by the raw string,
    // a zero terminator and then finally followed by any zeros needed to fill
    // in the rest of the hglobal (if we got one bigger than we asked for).
    //

    * (WCHAR *) pStrHtmlText = NATIVE_UNICODE_SIGNATURE;
    
    memcpy( pStrHtmlText + sizeof( WCHAR ), pStr, cbStr );
    
    memset(
        pStrHtmlText + cbStr + sizeof( WCHAR ),
        0, GlobalSize( *phglobal ) - cbStr - sizeof( WCHAR ) );

    GlobalUnlock(*phglobal);

Cleanup:

    RRETURN( hr );
    
Error:

    if (*phglobal)
        GlobalFree( *phglobal );
    
    *phglobal = 0;

    goto Cleanup;
}
