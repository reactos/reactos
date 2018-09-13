//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       ccompapi.cxx
//
//  Contents:   common compobj API Worker routines used by com, stg, scm etc
//
//  Classes:
//
//  Functions:
//
//  History:    31-Dec-93   ErikGav     Chicago port
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <ole2sp.h>
#include <ole2com.h>


NAME_SEG(CompApi)
ASSERTDATA

static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const WCHAR wszDigits[] = L"0123456789ABCDEF";


//+-------------------------------------------------------------------------
//
//  Function:   wStringFromGUID2     (internal)
//
//  Synopsis:   converts GUID into {...} form without leading identifier;
//
//  Arguments:  [rguid] - the guid to convert
//              [lpszy] - buffer to hold the results
//              [cbMax] - sizeof the buffer
//
//  Returns:    amount of data copied to lpsz if successful
//              0 if buffer too small.
//
//--------------------------------------------------------------------------
INTERNAL_(int)  wStringFromGUID2(REFGUID rguid, LPWSTR lpsz, int cbMax)
{
    int i;
    LPWSTR p = lpsz;

    const BYTE * pBytes = (const BYTE *) &rguid;

    *p++ = L'{';

    for (i = 0; i < sizeof(GuidMap); i++)
    {
        if (GuidMap[i] == '-')
        {
            *p++ = L'-';
        }
        else
        {
            *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
        }
    }
    *p++ = L'}';
    *p   = L'\0';

    return GUIDSTR_MAX;
}

#ifdef _CHICAGO_

static const CHAR szDigits[] = "0123456789ABCDEF";
//+-------------------------------------------------------------------------
//
//  Function:   wStringFromGUID2A     (internal)
//
//  Synopsis:   Ansi version of wStringFromGUID2 (for Win95 Optimizations)
//
//  Arguments:  [rguid] - the guid to convert
//              [lpszy] - buffer to hold the results
//              [cbMax] - sizeof the buffer
//
//  Returns:    amount of data copied to lpsz if successful
//              0 if buffer too small.
//
//--------------------------------------------------------------------------

INTERNAL_(int) wStringFromGUID2A(REFGUID rguid, LPSTR lpsz, int cbMax)  // internal
{
    int i;
    LPSTR p = lpsz;

    const BYTE * pBytes = (const BYTE *) &rguid;

    *p++ = '{';

    for (i = 0; i < sizeof(GuidMap); i++)
    {
        if (GuidMap[i] == '-')
        {
            *p++ = '-';
        }
        else
        {
            *p++ = szDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *p++ = szDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
        }
    }
    *p++ = '}';
    *p   = '\0';

    return GUIDSTR_MAX;
}
#endif


