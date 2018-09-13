//+---------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1993 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       dll.cxx
//
//  Contents:   Debug DLL stubs
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


extern "C" BOOL __cdecl DbgDllMain(HANDLE hinst, DWORD dwReason, LPVOID lpReason);

#ifdef BUILD_XMLDBG_AS_LIB
DWORD g_dwFALSE = 0;
#endif

BOOL
DllMain(HANDLE hinst, DWORD dwReason, LPVOID lpReason)
{
    return DbgDllMain(hinst, dwReason, lpReason);
}

