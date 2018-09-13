/*
 * @(#)common.hxx 1.0 2/28/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _COMMON_HXX
#define _COMMON_HXX


#define NOVTABLE __declspec(novtable)

#ifdef MSXML_EXPORT
#define DLLEXPORT   __declspec( dllexport )
#else
#define DLLEXPORT
#endif

#define LENGTH(A) (sizeof(A)/sizeof(A[0]))

#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <objbase.h>

#include "debug/include/msxmldbg.h"
#include "memutil.h"
#include "core/com/_unknown.hxx"
//#include "_reference.hxx"
//#include "cslock.hxx"
#include "staticunknown.hxx"

#define checkhr2(a)  { hr = a; if (FAILED(hr)) return hr; }

#define checkerr(a) { hr = a; if (FAILED(hr)) goto error; }

WCHAR * StringDup(const WCHAR * s);
WCHAR * StringDupHR(const WCHAR * s, HRESULT *);

inline ULONG StrLen(const WCHAR * s)
{
    return lstrlen(s);
}

IUnknown* SafeRelease(IUnknown* p);

#define XMLFLAG_RUNBUFFERONLY   0x1000

#endif _COMMON_HXX
