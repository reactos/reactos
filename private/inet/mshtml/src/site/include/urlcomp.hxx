//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       urlcomp.hxx
//
//  Contents:   URL compatibility code
//
//              Provides compatibility bits for specific URLs
//
//----------------------------------------------------------------------------

#ifndef I_URLCOMP_HXX_
#define I_URLCOMP_HXX_
#pragma INCMSG("--- Beg 'urlcomp.hxx'")

#define URLCOMPAT_NODEFAULT    0x00000001
#define URLCOMPAT_KEYDOWN      0x00000004

HRESULT CompatBitsFromUrl(TCHAR *pchUrl, DWORD *dwUrlCompat);

#pragma INCMSG("--- End 'urlcomp.hxx'")
#else
#pragma INCMSG("*** Dup 'urlcomp.hxx'")
#endif
