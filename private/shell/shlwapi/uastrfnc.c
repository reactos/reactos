//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       uastrfnc.cpp
//
//  Contents:   Unaligned UNICODE lstr functions for MIPS, PPC, ALPHA
//
//  Classes:
//
//  Functions:
//
//  History:    1-11-95   davepl   Created
//
//--------------------------------------------------------------------------

// BUGBUG (DavePl) None of these pay any heed to locale!

#include "priv.h"
#pragma  hdrstop

#ifdef ALIGNMENT_MACHINE

#ifdef UNIX
#define _alloca alloca
#endif /* UNIX */

//+-------------------------------------------------------------------------
//
//  Function:   ualstrcpynW
//
//  Synopsis:   UNALIGNED UNICODE version of lstrcpyn
//
//  Arguments:  [lpString1]  -- dest string
//              [lpString2]  -- source string
//              [iMaxLength] -- max chars to copy
//
//  Returns:
//
//  History:    1-11-95   davepl   NT Port
//
//  Notes:
//
//--------------------------------------------------------------------------

UNALIGNED WCHAR *  ualstrcpynW(UNALIGNED WCHAR * lpString1,
                               UNALIGNED const WCHAR * lpString2,
                               int iMaxLength)
{
    UNALIGNED WCHAR * src;
    UNALIGNED WCHAR * dst;

    src = (UNALIGNED WCHAR *)lpString2;
    dst = lpString1;

    while(iMaxLength && *src)
    {
        *dst++ = *src++;
        iMaxLength--;
    }

    if ( iMaxLength )
    {
        *dst = '\0';
    }
    else
    {
        dst--;
        *dst = '\0';
    }
    return dst;
}

//+-------------------------------------------------------------------------
//
//  Function:   ualstrcmpiW
//
//  Synopsis:   UNALIGNED UNICODE wrap of lstrcmpi
//
//  Arguments:  [dst] -- first string
//              [src] -- string to comapre to
//
//  Returns:
//
//  History:    1-11-95   davepl   Created
//              4-20-98   scotthan localized, bug 141655
//
//--------------------------------------------------------------------------
int ualstrcmpiW (UNALIGNED const WCHAR * dst, UNALIGNED const WCHAR * src)
{
    WCHAR  *pwszDst, *pwszSrc; 
    int    cb ;

    //  Make aligned copies on the stack if appropriate...
    //  BUGBUG - not the most inefficient tact (scotthan)
    //  note: _alloca should always succeed, unless out of stack
    if( IS_ALIGNED( dst ) )
        pwszDst = (WCHAR*)dst ;
    else
    {
        cb = (ualstrlenW( dst ) + 1) * sizeof(WCHAR) ;
        pwszDst = (WCHAR*)_alloca( cb ) ;
        CopyMemory( pwszDst, dst, cb ) ;
    }

    if( IS_ALIGNED( src ) )
        pwszSrc = (WCHAR*)src ;
    else
    {
        cb = (ualstrlenW( src ) + 1) * sizeof(WCHAR) ;
        pwszSrc = (WCHAR*)_alloca( cb ) ;
        CopyMemory( pwszSrc, src, cb ) ;
    }

    //  Call the aligned method.
    //  Note: if this ever runs on Win95, we should call StrCmpIW instead.
    return lstrcmpiW( pwszDst, pwszSrc ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   ualstrcmpW
//
//  Synopsis:   UNALIGNED UNICODE wrap of lstrcmp
//
//  Arguments:  [dst] -- first string
//              [src] -- string to comapre to
//
//  Returns:
//
//  History:    1-11-95   davepl   Created
//              4-29-98   scotthan localized, bug 164091
//
//--------------------------------------------------------------------------
int ualstrcmpW (UNALIGNED const WCHAR * src, UNALIGNED const WCHAR * dst)
{
    WCHAR  *pwszDst, *pwszSrc; 
    int    cb ;

    //  Make aligned copies on the stack if appropriate...
    //  BUGBUG - not the most inefficient tact (scotthan)
    //  note: _alloca should always succeed, unless out of stack
    if( IS_ALIGNED( dst ) )
        pwszDst = (WCHAR*)dst ;
    else
    {
        cb = (ualstrlenW( dst ) + 1) * sizeof(WCHAR) ;
        pwszDst = (WCHAR*)_alloca( cb ) ;
        CopyMemory( pwszDst, dst, cb ) ;
    }

    if( IS_ALIGNED( src ) )
        pwszSrc = (WCHAR*)src ;
    else
    {
        cb = (ualstrlenW( src ) + 1) * sizeof(WCHAR) ;
        pwszSrc = (WCHAR*)_alloca( cb ) ;
        CopyMemory( pwszSrc, src, cb ) ;
    }

    //  Call the aligned method.
    //  Note: if this ever runs on Win95, we should call StrCmpW instead.
    return lstrcmpW( pwszDst, pwszSrc ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   ualstrlenW
//
//  Synopsis:   UNALIGNED UNICODE version of lstrlen
//
//  Arguments:  [wcs] -- string to return length of
//
//  Returns:
//
//  History:    1-11-95   davepl   NT Port
//
//  Notes:
//
//--------------------------------------------------------------------------

size_t ualstrlenW (UNALIGNED const WCHAR * wcs)
{
    UNALIGNED const WCHAR *eos = wcs;

    while( *eos++ )
    {
        NULL;
    }

    return( (size_t) (eos - wcs - 1) );
}

//+-------------------------------------------------------------------------
//
//  Function:   ualstrcpyW
//
//  Synopsis:   UNALIGNED UNICODE version of lstrcpy
//
//  Arguments:  [dst] -- string to copy to
//              [src] -- string to copy from
//
//  Returns:
//
//  History:    1-11-95   davepl   NT Port
//
//  Notes:
//
//--------------------------------------------------------------------------

UNALIGNED WCHAR * ualstrcpyW(UNALIGNED WCHAR * dst, UNALIGNED const WCHAR * src)
{
    UNALIGNED WCHAR * cp = dst;

    while( *cp++ = *src++ )
        NULL ;

    return( dst );
}

#endif // ALIGNMENT_MACHINE
