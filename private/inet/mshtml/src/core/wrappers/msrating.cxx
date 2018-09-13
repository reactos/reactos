//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       msrating.cxx
//
//  Contents:   Dynamic wrappers for msrating functions
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef UNIX // UNIX doesn't use this file

#ifndef X_MSRATING_HXX_
#define X_MSRATING_HXX_
#include "msrating.hxx"
#endif

DYNLIB g_dynlibMSRATING = { NULL, NULL, "MSRATING.DLL" };

#define WRAPIT(wrapper, fn, a1, a2)\
STDAPI wrapper a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibMSRATING, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}

WRAPIT(AreRatingsEnabled, RatingEnabledQuery, (), ());

#endif // UNIX doesn't use this file.

