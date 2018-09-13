/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    util.h

Abstract:

    This is the include file for the utility functions.

Author:

    Dave Hastings (daveh) creation-date 10-Nov-1997

Revision History:

--*/
#ifndef _util_h_
#define _util_h_

#if DBG

#define ExitGracefully(hr, result, text) \
    {OutputDebugString(L##text); hr = result; goto exit_gracefully;}

#else
#define ExitGracefully(hr, result, text) \
    {hr = result; goto exit_gracefully;}

#endif

#if DBG

#define FailGracefully(hr, text) \
    {if (!SUCCEEDED(hr)){OutputDebugString(L##text); goto exit_gracefully;}}

#else
#define FailGracefully(hr, result, text) \
    {if (!SUCCEEDED(hr)){goto exit_gracefully;}}

#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

HRESULT
HandleQueryInterface(
    REFIID riid,
    PVOID *ppvInterface,
    INTERFACES *Interfaces,
    ULONG ArraySize
    );

VOID
PopulateListView(
    HWND ListView,
    PACKAGEDISPINFO *Packages,
    ULONG NumberOfPackages
    );

#endif