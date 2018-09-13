//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       user32.hxx
//
//  Contents:   Dynamic wrappers for user32.dll.
//
//----------------------------------------------------------------------------

#ifndef _USER32_HXX_
#define _USER32_HXX_

DYNLIB g_dynlibUSER32 = { NULL, NULL, "USER32.DLL" };


#define WRAPIT(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibUSER32, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR_NOTRACE((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1((hr), S_FALSE); \
}

#define WRAPIT_BOOL(fn, a1, a2)\
STDAPI_(BOOL) fn a1\
{\
    HRESULT hr; \
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibUSER32, #fn }; \
    hr = THR(LoadProcedure(&s_dynproc##fn)); \
    if (hr)\
        goto Cleanup;\
    return ((*(BOOL (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2); \
Cleanup:\
    RRETURN1((hr), S_FALSE); \
}

WRAPIT_BOOL(AllowSetForegroundWindow,
    (DWORD dwProcessId),
    (dwProcessId))

#endif  // _USER32_HXX_