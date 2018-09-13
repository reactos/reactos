//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       mapi.cxx
//
//  Contents:   Dynamic wrappers for URL monikers.
//
//----------------------------------------------------------------------------

#define _INTERNAL_WRAPPER_BYPASS
#include <padhead.hxx>

DYNLIB g_dynlibMAPI = { NULL, NULL, "MAPI32.DLL" };

#define WRAPIT(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMAPI, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}

#define WRAPIT_VOID(fn, a1, a2)\
VOID WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMAPI, #fn };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        return;\
    (*(VOID (APIENTRY *) a1)s_dynproc##fn.pfn) a2;\
}

#define WRAPIT_(type, fn, a1, a2)\
type WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMAPI, #fn };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        Assert(FALSE);\
    return (*(type (APIENTRY *) a1)s_dynproc##fn.pfn) a2;\
}

#define WRAPIT_FASTCALL(fn, cbarg, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMAPI, #fn "@" #cbarg };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}

#define WRAPIT_FASTCALL_VOID(fn, cbarg, a1, a2)\
VOID WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMAPI, #fn "@" #cbarg };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        return;\
    (*(VOID (APIENTRY *) a1)s_dynproc##fn.pfn) a2;\
}

#define WRAPIT_FASTCALL_(type, fn, cbarg, a1, a2)\
type WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMAPI, #fn "@" #cbarg };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        Assert(FALSE);\
    return (*(type (APIENTRY *) a1)s_dynproc##fn.pfn) a2;\
}




WRAPIT(MAPIInitialize,
    (LPVOID lpMapiInit),
    (lpMapiInit))

WRAPIT_VOID(MAPIUninitialize,
    (),
    ())

WRAPIT(MAPIAllocateBuffer,
    (ULONG cbSize, LPVOID FAR * lppBuffer),
    (cbSize, lppBuffer))

WRAPIT(MAPIAllocateMore,
    (ULONG cbSize, LPVOID lpObject, LPVOID FAR * lppBuffer),
    (cbSize, lpObject, lppBuffer))

WRAPIT(MAPIOpenLocalFormContainer,
    (LPMAPIFORMCONTAINER FAR * ppfcnt),
    (ppfcnt))
   
WRAPIT_(ULONG,
    MAPIFreeBuffer,
    (LPVOID lpBuffer),
    (lpBuffer))

WRAPIT_VOID(FreeProws,
    (LPSRowSet prows),
    (prows))

WRAPIT_FASTCALL(HrQueryAllRows,
    24,
    (LPMAPITABLE ptable, LPSPropTagArray ptaga, LPSRestriction pres, 
     LPSSortOrderSet psos, LONG crowsMax, LPSRowSet FAR * pprows),
    (ptable, ptaga, pres, psos, crowsMax, pprows))

WRAPIT_FASTCALL(HrGetOneProp,
    12,
    (LPMAPIPROP pmp, ULONG ulPropTag, LPSPropValue FAR * ppprop),
    (pmp, ulPropTag, ppprop))

WRAPIT_FASTCALL_VOID(FreePadrlist,
     4,
    (LPADRLIST padrlist),
    (padrlist))

WRAPIT_FASTCALL_(FILETIME,
    FtAddFt,
    16,
    (FILETIME Addend1, FILETIME Addend2),
    (Addend1, Addend2))

WRAPIT_FASTCALL_(FILETIME, 
    FtSubFt,
    16,
    (FILETIME Addend1, FILETIME Addend2),
    (Addend1, Addend2))

WRAPIT(WrapCompressedRTFStream,
    (LPSTREAM lpCRTFS, ULONG ulFlags, LPSTREAM FAR * lpUncomp),
    (lpCRTFS, ulFlags, lpUncomp))
