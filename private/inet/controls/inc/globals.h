//=--------------------------------------------------------------------------=
// Globals.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains externs and stuff for Global variables, etc ..
//
#ifndef _GLOBALS_H_

// the library that we are
//
extern const CLSID *g_pLibid;

//=--------------------------------------------------------------------------=
// support for licensing
//
extern BOOL   g_fMachineHasLicense;
extern BOOL   g_fCheckedForLicense;

//=--------------------------------------------------------------------------=
// does our server have a type library?
//
extern BOOL   g_fServerHasTypeLibrary;

//=--------------------------------------------------------------------------=
// our instance handle, and various pieces of information interesting to
// localization
//
extern HINSTANCE    g_hInstance;

extern const VARIANT_BOOL g_fSatelliteLocalization;
extern VARIANT_BOOL       g_fHaveLocale;
extern LCID               g_lcidLocale;

//=--------------------------------------------------------------------------=
// apartment threading support.
//
extern CRITICAL_SECTION g_CriticalSection;

//=--------------------------------------------------------------------------=
// our global memory allocator and global memory pool
//
extern HANDLE   g_hHeap;

//=--------------------------------------------------------------------------=
// global parking window for parenting various things.
//
extern HWND     g_hwndParking;

//=--------------------------------------------------------------------------=
// system information
//
extern BOOL g_fSysWin95;                    // we're under Win95 system, not just NT SUR
extern BOOL g_fSysWinNT;                    // we're under some form of Windows NT
extern BOOL g_fSysWin95Shell;               // we're under Win95 or Windows NT SUR { > 3/51)

#define _GLOBALS_H_
#endif // _GLOBALS_H_

