//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       usp.cxx
//
//  Contents:   Dynamic wrappers for USP.dll (UniScribe) functions.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_USP_HXX_
#define X_USP_HXX_
#include "usp.hxx"
#endif

DYNLIB g_dynlibUSP = { NULL, NULL, "USP10.DLL" };

#define WRAPIT(fn, a1, a2)\
HRESULT WINAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibUSP, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (*) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN(hr);\
}

#define WRAPIT_(type, fn, a1, a2)\
type WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibUSP, #fn };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        return NULL;\
    return (*(type (APIENTRY *) a1)s_dynproc##fn.pfn) a2;\
}

#define WRAPIT_ERR1(fn, a1, a2, e1)\
HRESULT WINAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibUSP, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (*) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, e1);\
}


WRAPIT(ScriptFreeCache,
    (SCRIPT_CACHE *psc),
    (psc))

WRAPIT_ERR1(ScriptItemize,
    (PCWSTR pwcInChars, int cInChars, int cMaxItems, const SCRIPT_CONTROL *psControl,
     const SCRIPT_STATE *psState, SCRIPT_ITEM *pItems, PINT pcItems),
    (pwcInChars, cInChars, cMaxItems, psControl, psState, pItems, pcItems),
    E_OUTOFMEMORY)

WRAPIT_ERR1(ScriptShape,
    (HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcChars, int cChars, int cMaxGlyphs,
     SCRIPT_ANALYSIS *psa, WORD *pwOutGlyphs, WORD *pwLogClust, SCRIPT_VISATTR *psva, int *pcGlyphs),
    (hdc, psc, pwcChars, cChars, cMaxGlyphs, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs),
    E_OUTOFMEMORY)

WRAPIT(ScriptPlace,
    (HDC hdc, SCRIPT_CACHE *psc, const WORD *pwGlyphs, int cGlyphs, const SCRIPT_VISATTR *psva,
     SCRIPT_ANALYSIS *psa, int *piAdvance, GOFFSET *pGoffset, ABC *pABC),
    (hdc, psc, pwGlyphs, cGlyphs, psva, psa, piAdvance, pGoffset, pABC))

WRAPIT(ScriptTextOut,
    (const HDC hdc, SCRIPT_CACHE *psc, int x, int y, UINT fuOptions, const RECT  *lprc, 
     const SCRIPT_ANALYSIS *psa, const WCHAR *pwcInChars, int cChars, const WORD *pwGlyphs, 
     int cGlyphs, const int *piAdvance, const int *piJustify, const GOFFSET *pGoffset),
    (hdc, psc, x, y, fuOptions, lprc, 
     psa, pwcInChars, cChars, pwGlyphs, 
     cGlyphs, piAdvance, piJustify, pGoffset))

WRAPIT(ScriptBreak,
    (PCWSTR pwcChars, int cChars,const SCRIPT_ANALYSIS *psa, SCRIPT_LOGATTR *psla),
    (pwcChars, cChars, psa, psla))

WRAPIT(ScriptGetCMap,
    (HDC hdc, SCRIPT_CACHE * psc, PCWSTR pwcInChars, int cChars, DWORD dwFlags, PWORD pwOutGlyphs),
    (hdc, psc, pwcInChars, cChars, dwFlags, pwOutGlyphs))

WRAPIT(ScriptGetProperties,
    (const SCRIPT_PROPERTIES ***ppSp, int *piNumScripts),
    (ppSp, piNumScripts))

WRAPIT(ScriptGetFontProperties,
    (HDC hdc, SCRIPT_CACHE *psc, SCRIPT_FONTPROPERTIES  *sfp),
    (hdc, psc, sfp))

WRAPIT(ScriptStringAnalyse,
    (HDC hdc, const void *pString, int cString, int cGlyphs, int iCharset, DWORD dwFlags,
     int iReqWidth, SCRIPT_CONTROL *psControl, SCRIPT_STATE *psState, const int *piDx, 
     SCRIPT_TABDEF *pTabdef, const BYTE *pbInClass, SCRIPT_STRING_ANALYSIS *pssa),
    (hdc, pString, cString, cGlyphs, iCharset, dwFlags, iReqWidth, psControl, psState,
     piDx, pTabdef, pbInClass, pssa))

WRAPIT(ScriptStringGetLogicalWidths,
    (SCRIPT_STRING_ANALYSIS ssa, int *piDx),
    (ssa, piDx))

WRAPIT(ScriptStringOut,
    (SCRIPT_STRING_ANALYSIS ssa, int iX, int iY, UINT uOptions, const RECT *prc,
     int iMinSel, int iMaxSel, BOOL fDisabled),
    (ssa, iX, iY, uOptions, prc, iMinSel, iMaxSel, fDisabled))

WRAPIT(ScriptStringFree,
    (SCRIPT_STRING_ANALYSIS *pssa),
    (pssa))

WRAPIT_(const SIZE*,
        ScriptString_pSize,
        (SCRIPT_STRING_ANALYSIS ssa),
        (ssa))

WRAPIT_(const int*,
        ScriptString_pcOutChars,
        (SCRIPT_STRING_ANALYSIS ssa),
        (ssa))


WRAPIT_(const SCRIPT_LOGATTR*,
        ScriptString_pLogAttr,
        (SCRIPT_STRING_ANALYSIS ssa),
        (ssa))

