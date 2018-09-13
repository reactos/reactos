//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       comdlg32.cxx
//
//  Contents:   Dynamic wrappers for common dialog procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_CDERR_H_
#define X_CDERR_H_
#include <cderr.h>
#endif

#ifdef WIN16
#ifndef X_COMMDLG_H_
#define X_COMMDLG_H_
#include <commdlg.h>
#endif

#ifndef X_DEFS16_H_
#define X_DEFS16_H_
#include "defs16.h"
#endif
#endif // WIN16

#ifndef WIN16
DYNLIB g_dynlibCOMDLG32 = { NULL, NULL, "COMDLG32.dll" };
#else
DYNLIB g_dynlibCOMDLG32 = { NULL, NULL, "COMMDLG.DLL" };
#endif // !WIN16

BOOL APIENTRY
ChooseColorW(LPCHOOSECOLORW lpcc)
{
    static DYNPROC s_dynprocChooseColorW =
            { NULL, &g_dynlibCOMDLG32, "ChooseColorW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocChooseColorW);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCHOOSECOLORW))s_dynprocChooseColorW.pfn)
            (lpcc);
}

BOOL APIENTRY
ChooseColorA(LPCHOOSECOLORA lpcc)
{
    static DYNPROC s_dynprocChooseColorA =
            { NULL, &g_dynlibCOMDLG32, "ChooseColorA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocChooseColorA);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCHOOSECOLORA))s_dynprocChooseColorA.pfn)
            (lpcc);
}


BOOL APIENTRY
ChooseFontW(LPCHOOSEFONTW lpcf)
{
    static DYNPROC s_dynprocChooseFontW =
            { NULL, &g_dynlibCOMDLG32, "ChooseFontW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocChooseFontW);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCHOOSEFONTW))s_dynprocChooseFontW.pfn)
            (lpcf);
}

BOOL APIENTRY
ChooseFontA(LPCHOOSEFONTA lpcf)
{
    static DYNPROC s_dynprocChooseFontA =
            { NULL, &g_dynlibCOMDLG32, "ChooseFontA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocChooseFontA);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCHOOSEFONTA))s_dynprocChooseFontA.pfn)
            (lpcf);
}

DWORD APIENTRY
CommDlgExtendedError()
{
    static DYNPROC s_dynprocCommDlgExtendedError =
            { NULL, &g_dynlibCOMDLG32, "CommDlgExtendedError" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocCommDlgExtendedError);
    if (hr)
        return CDERR_INITIALIZATION;

    return ((DWORD (APIENTRY *)())s_dynprocCommDlgExtendedError.pfn)
            ();
}

#ifdef UNIX
extern "C" char *APIENTRY
MwFilterType(char *filter, BOOL b)
{
    static DYNPROC s_dynprocMwFilterType =
            { NULL, &g_dynlibCOMDLG32, "MwFilterType" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocMwFilterType);
    if (hr)
        return NULL;

    return ((char *(APIENTRY *)(char *, BOOL))s_dynprocMwFilterType.pfn)
            (filter, b);
}
#endif // UNIX


#if !defined(_M_IX86_)

BOOL APIENTRY
GetOpenFileNameW(LPOPENFILENAMEW pofnw)
{
    static DYNPROC s_dynprocGetOpenFileNameW =
            { NULL, &g_dynlibCOMDLG32, "GetOpenFileNameW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetOpenFileNameW);
    if (hr)
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPOPENFILENAMEW))s_dynprocGetOpenFileNameW.pfn)
            (pofnw);
}

BOOL APIENTRY
GetSaveFileNameW(LPOPENFILENAMEW pofnw)
{
    static DYNPROC s_dynprocGetSaveFileNameW =
            { NULL, &g_dynlibCOMDLG32, "GetSaveFileNameW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocGetSaveFileNameW);
    if (hr)
        return FALSE;

    return ((BOOL (APIENTRY *)(LPOPENFILENAMEW))s_dynprocGetSaveFileNameW.pfn)
            (pofnw);
}

#endif
