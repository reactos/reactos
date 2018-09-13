//+---------------------------------------------------------------------------
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       src\core\include\platrisc.h
//
//  Contents:   header file containing inline declarations for all Win95
//              functions used in Forms^3.  For RISC platforms ONLY.
//
//  History:    02-Nov-94   SumitC      Created
//
//----------------------------------------------------------------------------

#ifndef I_PLATRISC_H_
#define I_PLATRISC_H_
#pragma INCMSG("--- Beg 'platrisc.h'")

//
//  Definitions for the init and uninit functions.  These are to be called
//  as the first thing during DLL attach and the last thing during DLL detach,
//  respectively.
//
void InitWrappers();
void DeinitWrappers();


// definitions for all non-Intel platforms, i.e. MIPS, Alpha, PowerPC etc.

extern DWORD g_dwPlatformVersion;   // (dwMajorVersion << 16) + (dwMinorVersion)
extern DWORD g_dwPlatformID;        // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT
extern BOOL g_fUnicodePlatform;

// All such platforms are Unicode-only, so there is no need for the Unicode
// wrapper functions.


#pragma INCMSG("--- End 'platrisc.h'")
#else
#pragma INCMSG("*** Dup 'platrisc.h'")
#endif
