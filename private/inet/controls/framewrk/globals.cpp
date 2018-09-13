//=--------------------------------------------------------------------------=
// Globals.C
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains global variables and strings and the like that just don't fit
// anywhere else.
//
#include "IPServer.H"

//=--------------------------------------------------------------------------=
// support for licensing
//
BOOL g_fMachineHasLicense;
BOOL g_fCheckedForLicense;

//=--------------------------------------------------------------------------=
// does our server have a type library?
//
BOOL g_fServerHasTypeLibrary = TRUE;

//=--------------------------------------------------------------------------=
// our instance handles
//
HINSTANCE    g_hInstance;
HINSTANCE    g_hInstResources;
VARIANT_BOOL g_fHaveLocale;

//=--------------------------------------------------------------------------=
// our global memory allocator and global memory pool
//
HANDLE   g_hHeap;

//=--------------------------------------------------------------------------=
// apartment threading support.
//
CRITICAL_SECTION    g_CriticalSection;

//=--------------------------------------------------------------------------=
// global parking window for parenting various things.
//
HWND     g_hwndParking;

//=--------------------------------------------------------------------------=
// system information
//
BOOL    g_fSysWin95;                    // we're under Win95 system, not just NT SUR
BOOL    g_fSysWinNT;                    // we're under some form of Windows NT
BOOL    g_fSysWin95Shell;               // we're under Win95 or Windows NT SUR { > 3/51)


