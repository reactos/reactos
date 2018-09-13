//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       hlink.cxx
//
//  Contents:   Dynamic wrappers for Hlink.dll functions
//
//----------------------------------------------------------------------------

#include "precomp.hxx"


#ifndef X_HLINK_H_
#define X_HLINK_H_
#include "hlink.h"
#endif

DYNLIB g_dynlibHLINK = { NULL, NULL, "HLINK.DLL" };

#define WRAPIT(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibHLINK, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}


WRAPIT(HlinkCreateFromMoniker,
    (IMoniker * pimkTrgt, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, IHlinkSite * pihlsite,
     DWORD dwSiteData, IUnknown * piunkOuter, REFIID riid, void ** ppvObj),
    (pimkTrgt, pwzLocation, pwzFriendlyName, pihlsite, dwSiteData, piunkOuter, riid, ppvObj))

WRAPIT(HlinkCreateFromString,
    (LPCWSTR pwzTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, IHlinkSite * pihlsite,
     DWORD dwSiteData, IUnknown * piunkOuter, REFIID riid, void ** ppvObj),
    (pwzTarget, pwzLocation, pwzFriendlyName, pihlsite, dwSiteData, piunkOuter, riid, ppvObj))

