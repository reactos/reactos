//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       T2EmWrap.hxx
//
//  Contents:   Wrapper functions for the OpenType Embedding DLL (t2embed.dll).
//
//----------------------------------------------------------------------------

#ifndef I_T2EMWRAP_HXX_
#define I_T2EMWRAP_HXX_
#pragma INCMSG("--- Beg 't2emwrap.hxx'")

#ifndef X_T2EMBAPI_H_
#define X_T2EMBAPI_H_
#include "t2embapi.h"
#endif

typedef T2API LONG (WINAPI *LOADEMBFONTFN)( HANDLE*, ULONG, ULONG*, ULONG, ULONG*, READEMBEDPROC, LPVOID, LPWSTR, LPSTR, TTLOADINFO* );
typedef T2API LONG (WINAPI *DELEMBFONTFN)( HANDLE, ULONG, ULONG* );

LONG WINAPI T2LoadEmbeddedFont( HANDLE *phFontReference, ULONG ulFlags, ULONG *pulPrivStatus,
        ULONG ulPrivs, ULONG *pulStatus, READEMBEDPROC lpfnReadFromStream, LPVOID lpvReadStream,
        LPWSTR szWinFamilyName, LPSTR szMacFamilyName, TTLOADINFO *pTTLoadInfo );

LONG WINAPI T2DeleteEmbeddedFont ( HANDLE hFontReference, ULONG ulFlags, ULONG *pulStatus );

#pragma INCMSG("--- End 't2emwrap.hxx'")
#else
#pragma INCMSG("*** Dup 't2emwrap.hxx'")
#endif
