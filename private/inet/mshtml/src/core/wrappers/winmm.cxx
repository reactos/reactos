//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       winmm.cxx
//
//  Contents:   Dynamic wrappers for multi media
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_MMSYSTEM_H_
#define X_MMSYSTEM_H_
#define _WINMM_
#include <mmsystem.h>
#endif

DYNLIB g_dynlibWINMM = { NULL, NULL, "WINMM.DLL" };

BOOL WINAPI
PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
    static DYNPROC s_dynprocPlaySoundA =
            { NULL, &g_dynlibWINMM, "PlaySoundA" };

    if (THR(LoadProcedure(&s_dynprocPlaySoundA)))
        return FALSE;

    return (*(BOOL (APIENTRY *)(LPCSTR, HMODULE, DWORD))s_dynprocPlaySoundA.pfn)
            (pszSound, hmod, fdwSound);

}
