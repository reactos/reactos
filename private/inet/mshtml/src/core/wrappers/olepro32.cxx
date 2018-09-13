//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       olepro32.cxx
//
//  Contents:   Dynamic wrappers for OLE Automation monikers.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef UNIX
DYNLIB g_dynlibOLEPRO32 = { NULL, NULL, "olepro32.dll" };
#else
DYNLIB g_dynlibOLEPRO32 = { NULL, NULL, "oleaut32.dll" };
#endif

#define WRAP_HR(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibOLEPRO32, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}

#define WRAP(t, fn, a1, a2)\
STDAPI_(t) fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibOLEPRO32, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        return 0;\
    return (*(t (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2;\
}

#define WRAP_VOID(fn, a1, a2)\
STDAPI_(void) fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibOLEPRO32, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        return;\
    (*(void (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2;\
}



WRAP_HR(OleLoadPicture, 
    (LPSTREAM lpstream, LONG lSize, BOOL fRunmode, REFIID riid, LPVOID FAR* lplpvObj),
    (lpstream, lSize, fRunmode, riid, lplpvObj))

WRAP_HR(OleLoadPicturePath, 
    (LPOLESTR  szURLorPath, LPUNKNOWN punkCaller, DWORD dwReserved, OLE_COLOR clrReserved, REFIID riid, LPVOID *  ppvRet),
    (szURLorPath, punkCaller, dwReserved, clrReserved, riid, ppvRet))

WRAP_HR(OleCreateFontIndirect,
    (LPFONTDESC lpFontDesc, REFIID riid, LPVOID FAR* lplpvObj),
    (lpFontDesc, riid, lplpvObj))

WRAP_HR(OleCreatePropertyFrame,
    (HWND hwndOwner, UINT x, UINT y, LPCOLESTR lpszCaption, ULONG cObjects, LPUNKNOWN FAR* ppUnk, ULONG cPages, LPCLSID pPageClsID, LCID lcid, DWORD dwReserved, LPVOID pvReserved),
    (hwndOwner, x, y, lpszCaption, cObjects, ppUnk, cPages, pPageClsID, lcid, dwReserved, pvReserved))
