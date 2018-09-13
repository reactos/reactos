//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       uastrfnc.c
//
//  Contents:   forwards calls to Unaligned UNICODE lstr functions 
//              for MIPS, PPC, ALPHA to shlwapi.dll
//
//  Classes:
//
//  Functions:
//
//  History:    1-11-95   davepl   Created
//
//--------------------------------------------------------------------------

// BUGBUG (DavePl) None of these pay any heed to locale!

#include "shellprv.h"
#pragma  hdrstop

#ifdef ALIGNMENT_MACHINE


UNALIGNED WCHAR *  SHLualstrcpynW(UNALIGNED WCHAR * lpString1,
                               UNALIGNED const WCHAR * lpString2,
                               int iMaxLength)
{
    return (ualstrcpynW (lpString1, lpString2, iMaxLength));
}

int SHLualstrcmpiW (UNALIGNED const WCHAR * dst, UNALIGNED const WCHAR * src)
{
    return (ualstrcmpiW (dst, src));
}

int SHLualstrcmpW (UNALIGNED const WCHAR * src, UNALIGNED const WCHAR * dst)
{
    return (ualstrcmpW (dst, src));
}

size_t SHLualstrlenW (UNALIGNED const WCHAR * wcs)
{
    return (ualstrlenW (wcs));
}

UNALIGNED WCHAR * SHLualstrcpyW(UNALIGNED WCHAR * dst, UNALIGNED const WCHAR * src)
{
    return (ualstrcpyW (dst, src));
}

#endif // ALIGNMENT_MACHINE
